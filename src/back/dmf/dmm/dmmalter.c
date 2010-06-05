/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <lo.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dudbms.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmmcb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dm0c.h>
#include    <dm2d.h>
#include    <dmm.h>

/**
**
** Name: DMMALTER.C - Functions needed to alter database config file.
**
** Description:
**	This file contains functions necessary for altering the config
**	file of a given database.  Currently there are two operations
**	provided: either you can update the table id, access, and service
**	fields of the config file (which DESTROYDB does), or you can
**	change the location type flags in the extent list (used for
**	changing a WORK locations defaultability).
**    
**      dmm_alter()       -  Routine to perform the alter operation.
**
** History:
**      20-mar-89 (derek) 
**          Created for Orange.
**	08-aug-1990 (ralph)
**	    Fixed destroydb problem when data directory not found.
**	    Corrected calls to ule_format.
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	 7-jul-1992 (rogerk)
**	    Prototyping DMF.
**	05-nov-92 (jrb)
**	    Changes for multi-location sorts.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system typedefs
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	20-apr-1994 (andyw)
**	    Error E_DM011A reported if the database directory does not exist 
**	    whilst doing a destroydb operation. An invalid check was used
**	    on dmm->dmm_cb_access against DU_DESTROYDB != 0. bug #60338
**	10-Nov-1994 (ramra01)
**		notify qef DB is open if DESTROYDB and lock resource is busy
**      03-jun-1996 (canor01)
**          For operating-system threads, remove external semaphore protection
**          for NMingpath().
**	08-may-1997 (wonst02)
** 	    If getting access to existing readonly database, skip the alter_db.
** 	29-may-1997 (wonst02)
** 	    Remove 08-may-1997 change; do appropriate change in dm2d_alter_db.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**      06-jun-2008 (stial01)
**          Changed DMM_UPDATE_CONFIG to use new dmm_upd_map (b120475)
**	05-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	18-Nov-2008 (jonj)
**	    dm0c_?, dm2d_? functions converted to DB_ERROR *
**	10-Dec-2008 (jonj)
**	    SIR 120874: Remove last vestiges of CL_SYS_ERR,
**	    old form uleFormat.
**      14-May-2010 (stial01)
**          Changes for Long IDs
**/

/*
** Forward Declarations
*/
static STATUS	list_func(VOID);


/*{
** Name: dmm_alter - Alters a databases configuration file.
**
**  INTERNAL DMF call format:      status = dmm_alter(&dmm_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMM_ALTER_DB, &dmm_cb);
**
** Description:
**      The dmm_alter function handles altering the current database
**      state maintained in the configuration file.  You can alter
**      the current database service type, the current database access type,
**      and the value of next table-id available for use, or you can alter
**	the status bits in the extent list.  Which operation is performed
**	depends on the value of dmm_alter_op.
**
** Inputs:
**      dmm_cb 
**          .type                           Must be set to DMM_MAINT_CB.
**          .length                         Must be at least sizeof(DMM_CB).
**	    .dmm_flags_mask		    Must be zero.
**          .dmm_tran_id                    Must be the current transaction 
**                                          identifier returned from the 
**                                          begin transaction operation.
**          .dmm_db_location.data_address   Pointer database location.
**	    .dmm_dblocation.data_in_size    The size of the database location.
**	    .dmm_alter_op		    Type of ALTER operation to perform.
**	    .dmm_loc_list		    Pointer to array of locations.
**                                          Each entry in array is of type
**                                          DB_LOC_NAME.  Must be zero if
**                                          no locations specified.
**  	    .dmm_db_service 	            Set to new value of database service
**  	    .dmm_db_access 	            Set to new value of database access
**	    .dmm_tbl_id                     Set to new value of next table id
** Output:
**      dmm_cb
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM0010_BAD_DB_ID
**                                          E_DM001A_BAD_FLAG
**                                          E_DM001D_BAD_LOCATION_NAME
**                                          E_DM002A_BAD_PARAMETER
**                                          E_DM003B_BAD_TRAN_ID
**                                          E_DM0042_DEADLOCK
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM0064_USER_INTR
**                                          E_DM0065_USER_ABORT
**                                          E_DM010C_TRAN_ABORTED
**
**         .error.err_data                  Set to attribute in error by 
**                                          returning index into attribute list.
**
** History:
**	20-mar-1989 (Derek)
**	    Created for ORANGE.
**	08-aug-1990 (Ralph)
**	    Fixed destroydb problem when data directory not found
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.  When update config file, write
**	    backup copy as well.
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**      8-nov-1992 (ed)
**          remove DB_MAXNAME dependency
**	05-nov-92 (jrb)
**	    Added switch stmt to handle a variety of operations; added
**	    DMM_EXT_ALTER operation to alter config file's location type
**	    bits in the extent array.  For multi-location sorts project.
**      12-oct-93 (jrb)
**          Moved bit twiddling code to dm2d; in order to log stuff we needed
**	    to create a transaction and other things, and this was better done
**          in dm2d.
**      20-apr-1994 (andyw)
**          Error E_DM011A reported if the database directory does not exist
**          whilst doing a destroydb operation. An invalid check was used
**          on dmm->dmm_cb_access against DU_DESTROYDB != 0. bug #60338
**	08-may-1997 (wonst02)
** 	    If getting access to existing readonly database, skip the alter_db.
** 	29-may-1997 (wonst02)
** 	    Remove 08-may-1997 change; do appropriate change in dm2d_alter_db.
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
**	12-Nov-2009 (kschendel) SIR 122882
**	    Re-interpret cmptlvl as u_i4 instead of char[4].
*/

static STATUS
list_func(VOID)
{
    return (FAIL);
}

DB_STATUS
dmm_alter(
DMM_CB    *dmm_cb)
{
    DMM_CB	    	*dmm = dmm_cb;
    DM0C_CNF	    	*cnf = 0;
    DM0C_EXT	    	*ext;
    DMP_DCB	    	dcb;
    DML_XCB	    	*xcb;
    DML_SCB	    	*scb;
    LK_LOCK_KEY	    	db_key;
    i4	    	db_length;
    i4	    	lock_flag;
    i4	    	physical_length;
    i4	    	i;
    i4	    	j;
    STATUS	    	del_error;
    STATUS	    	status;
    i4	    		*err_code = &dmm->error.err_code;
    char	    	*physical_name;
    char	    	location_buffer[132];
    char	    	db_buffer[sizeof(DB_DB_NAME)+1];
    LOCATION	    	cl_loc;
    CL_ERR_DESC	    	sys_err;
    DI_IO	    	di_context;
    DM2D_ALTER_INFO 	dm2d_alter_info;

    CLRDBERR(&dmm->error);

    /*	Check arguments. */

    /*  Check Transaction Id. */

    xcb = (DML_XCB *)dmm->dmm_tran_id;
    if (dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) == E_DB_OK)
    {
	scb = xcb->xcb_scb_ptr;

	/* Check for external interrupts */
	if ( scb->scb_ui_state )
	    dmxCheckForInterrupt(xcb, err_code);
	if ( xcb->xcb_state )
	{
	    if (xcb->xcb_state & XCB_USER_INTR)
		SETDBERR(&dmm->error, 0, E_DM0065_USER_INTR);
	    else if (xcb->xcb_state & XCB_FORCE_ABORT)
		SETDBERR(&dmm->error, 0, E_DM010C_TRAN_ABORTED);
	    else if (xcb->xcb_state & XCB_ABORT)
		SETDBERR(&dmm->error, 0, E_DM0064_USER_ABORT);
	}
    }
    else
	SETDBERR(&dmm->error, 0, E_DM003B_BAD_TRAN_ID);

    /*  Check Location. */

    if ( dmm->error.err_code == 0 &&
	(dmm->dmm_db_location.data_in_size == 0 ||
	 dmm->dmm_db_location.data_address == 0) )
    {
	SETDBERR(&dmm->error, 0, E_DM0072_NO_LOCATION);
    }

    if ( dmm->error.err_code )
	return (E_DB_ERROR);

    /* Get db name and location name into local variables */

    MEcopy(dmm->dmm_db_location.data_address,
	dmm->dmm_db_location.data_in_size,
	location_buffer);
    location_buffer[dmm->dmm_db_location.data_in_size] = 0;
    MEcopy(dmm->dmm_db_name.db_db_name, sizeof(DB_DB_NAME), db_buffer);
    db_buffer[sizeof(DB_DB_NAME)] = 0;
    STtrmwhite(db_buffer);
    db_length = STlength(db_buffer);
    LOingpath(location_buffer, db_buffer, LO_DB, &cl_loc);
    LOtos(&cl_loc, &physical_name);
    physical_length = STlength(physical_name);
    MEmove(physical_length, physical_name, ' ',
	sizeof(dcb.dcb_location.physical), (PTR)&dcb.dcb_location.physical);

    /* Check that location exists */

    status = DIlistfile(&di_context, physical_name, physical_length,
	list_func, 0, &sys_err);
    if (status != DI_ENDFILE)
    {
	/* OK if dir not found when destroying the database */
	if ((status == DI_DIRNOTFOUND) &&
	    ((dmm->dmm_db_access & DU_DESTROYDB) == 0))
	{
	    return (E_DB_OK);
	}
	SETDBERR(&dmm->error, 0, E_DM011A_ERROR_DB_DIR);
	return (E_DB_ERROR);
    }
    
    /* Fill in dm2d_alter_info struct in preparation for calling dm2d_alter_db
    **
    ** Currently we ask for logging only if the operation is DMM_EXT_ALTER.
    ** We always do locking.
    **
    ** Note: we do neither logging nor locking when calling this routine
    ** 	     for rollforward.
    */

    dm2d_alter_info.lock_list     = scb->scb_lock_list;
    dm2d_alter_info.lock_no_wait  = (dmm->dmm_flags_mask & DMM_NOWAIT) != 0;
    dm2d_alter_info.logging       = dmm->dmm_alter_op == DMM_EXT_ALTER;
    dm2d_alter_info.locking       = 1;
    dm2d_alter_info.name          = &dmm->dmm_db_name;
    dm2d_alter_info.db_loc	  = physical_name;
    dm2d_alter_info.l_db_loc      = physical_length;
    dm2d_alter_info.location_name = &dmm->dmm_location_name;
	
    /* Convert DMM constants to DM2D equivalents and fill in operation-
    ** specific info
    */
    if (dmm->dmm_alter_op == DMM_TAS_ALTER)
    {
	dm2d_alter_info.alter_op = DM2D_TAS_ALTER;
	dm2d_alter_info.alter_info.tas_info.db_service = dmm->dmm_db_service;
	dm2d_alter_info.alter_info.tas_info.db_access = dmm->dmm_db_access;
	dm2d_alter_info.alter_info.tas_info.tbl_id = dmm->dmm_tbl_id;
    }
    else if (dmm->dmm_alter_op == DMM_EXT_ALTER)
    {
	i4		dtype, atype;

	dm2d_alter_info.alter_op = DM2D_EXT_ALTER;

	dtype = ((DMM_LOC_LIST **)dmm->dmm_loc_list.ptr_address)[0]->loc_type;
	atype = ((DMM_LOC_LIST **)dmm->dmm_loc_list.ptr_address)[1]->loc_type;

	if (dtype == DMM_L_WRK)
	    dm2d_alter_info.alter_info.ext_info.drop_loc_type = DCB_WORK;
	else if (dtype == DMM_L_AWRK)
	    dm2d_alter_info.alter_info.ext_info.drop_loc_type = DCB_AWORK;
	else
	    dm2d_alter_info.alter_info.ext_info.drop_loc_type = 0;
	    
	if (atype == DMM_L_WRK)
	    dm2d_alter_info.alter_info.ext_info.add_loc_type = DCB_WORK;
	else if (atype == DMM_L_AWRK)
	    dm2d_alter_info.alter_info.ext_info.add_loc_type = DCB_AWORK;
	else
	    dm2d_alter_info.alter_info.ext_info.add_loc_type = 0;
    }

    status = dm2d_alter_db(&dm2d_alter_info, &dmm->error);

    if (status == E_DB_OK)
	return (E_DB_OK);

    if ((dmm->error.err_code == E_DM012A_CONFIG_NOT_FOUND) &&
	((dmm->dmm_db_access & DU_DESTROYDB) != 0))
    {
	CLRDBERR(&dmm->error);
	return (E_DB_OK);
    }

    if (dmm->error.err_code > E_DM_INTERNAL)
    {
	uleFormat(&dmm->error, 0, NULL, ULE_LOG, 
		    NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(&dmm->error, 0, E_DM011D_ERROR_DB_ALTER);
    }

    if ((dmm->error.err_code == E_DM004C_LOCK_RESOURCE_BUSY) &&
	((dmm->dmm_db_access & DU_DESTROYDB) == 0))
    {
	SETDBERR(&dmm->error, 0, E_DM003F_DB_OPEN);
    }

    return (E_DB_ERROR);
}

/*{
** Name: dmm_config - Access/Alter field(s) in the config file header record.
**
**  INTERNAL DMF call format:      status = dmm_config(&dmm_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMM_CONFIG, &dmm_cb);
**
** Description:
**      The dmm_config function may read a single i4 from the config
**	file header:  cnf->dsc_status, cnf->dsc_dbcmptlvl, 
**		      cnf->dsc_1dbcmptminor, cnf->dsc_dbaccess or 
**		      cnf->dsc_dbservice.
**	Note:  The restriction of returning a single values is a work around to 
**	       the fact that byref parameters for procedures will not be 
**	       available in time to use for 6.5 development.  Eventually, it
**	       would be good to change the internal procedure invoking 
**	       dmm_config to return as many of these as desired at the same
**	       time.  In the meanwhile, returning one at a time is better than
**	       not being able to access them at all.
**	       THIS SHOULD EVENTUALLY BE CHANGED TO WORK WITH BYREF PARAMETER
**	       PASSING.
**
**	The dmm_config function may also be used to udpate any of the following
**	config file header fields:  cnf->dsc_status, cnf->dsc_dbcmptlvl, 
**				    cnf->dsc_1dbcmptminor, cnf->dsc_dbaccess or 
**				    cnf->dsc_dbservice.
**
**	Note that dmm_config should only be called if the server is attached to
**	an open database.  Therefore, this routine does NOT take a lock on the
**	database before accessing the config file.
**
** Inputs:
**      dmm_cb 
**          .type		    Must be set to DMM_MAINT_CB.
**          .length                 Must be at least sizeof(DMM_CB).
**	    .dmm_flags_mask	    May contain any of the following:
**					DMM_1GET_STATUS	    read dsc_status
**					DMM_2GET_CMPTLVL    read dsc_dbcmptlvl
**					DMM_3GET_CMPTMIN    read dsc_1dbcmptminor
**					DMM_4GET_ACCESS	    read dsc_dbaccess
**					DMM_5GET_SERVICE    read dsc_dbservice
**					DMM_UPDATE_CONFIG   update config file
**          .dmm_tran_id	    Must be the current transaction 
**                                  identifier returned from the 
**                                  begin transaction operation.
**          .dmu_location	    Pointer to array of locations.
**		.data_address	    Each entry in array is of type DB_LOC_NAME.
**				    Must be zero if no locations specified.
**		.data_in_size       The size of the location array in bytes.
**  	    dmm_db_service	    Value to set cnf->dsc_dbservice to.
**  	    dmm_db_access 	    Value to set cnf->dsc_dbaccess to.
**	    dmm_status		    value to set cnf->dsc_status to
**	    dmm_cmptlvl		    Value to set cnf->dsc_dbcmptlvl to
**	    dmm_dbcmptminor	    Value to set cnf->dsc_1dbcmptminor to
**	    dmm_cversion	    Value to set cnf->dsc_c_version to
**	    
** Output:
**      dmm_cb
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM0010_BAD_DB_ID
**                                          E_DM001A_BAD_FLAG
**                                          E_DM001D_BAD_LOCATION_NAME
**                                          E_DM002A_BAD_PARAMETER
**                                          E_DM003B_BAD_TRAN_ID
**                                          E_DM0042_DEADLOCK
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM0064_USER_INTR
**                                          E_DM0065_USER_ABORT
**                                          E_DM010C_TRAN_ABORTED
**
** History:
**	02-oct-92 (teresa)
**	    Initial creation
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	12-Mar-2001 (jenjo02)
**	    Don't update config file on close unless it was updated.
**	    The format of the file may be changing (upgradedb) and
**	    the file may not match compiled structures.
**	29-Dec-2004 (schka24)
**	    Allow updating of dsc_c_version.
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
*/
DB_STATUS
dmm_config(
DMM_CB    *dmm_cb)
{
    DMM_CB	    *dmm = dmm_cb;
    DM0C_CNF	    *cnf = 0;
    DM0C_EXT	    *ext;
    DMP_DCB	    dcb;
    DML_XCB	    *xcb;
    DML_SCB	    *scb;
    LK_LOCK_KEY	    db_key;
    i4	    db_length;
    i4	    lock_flag;
    i4	    physical_length;
    i4	    i;
    i4	    j;
    STATUS	    del_error;
    STATUS	    status;
    i4		    *err_code = &dmm->error.err_code;
    i4		    local_error;
    char	    *physical_name;
    char	    location_buffer[132];
    char	    db_buffer[sizeof(DB_DB_NAME)+1];
    LOCATION	    cl_loc;
    CL_ERR_DESC	    sys_err;
    DI_IO	    di_context;
    DB_ERROR	    local_dberr;
    i4	    *retval = &dmm->dmm_count;	/* return value */

    CLRDBERR(&dmm->error);

    /*	Check arguments. */

    /*  Check Transaction Id. */

    xcb = (DML_XCB *)dmm->dmm_tran_id;
    if (dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) == E_DB_OK)
    {
	scb = xcb->xcb_scb_ptr;

	/* Check for external interrupts */
	if ( scb->scb_ui_state )
	    dmxCheckForInterrupt(xcb, err_code);
	if ( xcb->xcb_state )
	{
	    if (xcb->xcb_state & XCB_USER_INTR)
		SETDBERR(&dmm->error, 0, E_DM0065_USER_INTR);
	    else if (xcb->xcb_state & XCB_FORCE_ABORT)
		SETDBERR(&dmm->error, 0, E_DM010C_TRAN_ABORTED);
	    else if (xcb->xcb_state & XCB_ABORT)
		SETDBERR(&dmm->error, 0, E_DM0064_USER_ABORT);
	}
    }
    else
	SETDBERR(&dmm->error, 0, E_DM003B_BAD_TRAN_ID);

    /*  Check Location. */

    if ( dmm->error.err_code == 0 &&
	(dmm->dmm_db_location.data_in_size == 0 ||
	 dmm->dmm_db_location.data_address == 0) )
    {
	SETDBERR(&dmm->error, 0, E_DM0072_NO_LOCATION);
    }

    if ( dmm->error.err_code )
	return (E_DB_ERROR);

    /*	Build a DCB so that me can use the DM0C routines. */

    MEfill(sizeof(dcb), 0, &dcb);
    MEcopy(dmm->dmm_db_location.data_address,
	dmm->dmm_db_location.data_in_size,
	location_buffer);
    location_buffer[dmm->dmm_db_location.data_in_size] = 0;
    MEcopy(dmm->dmm_db_name.db_db_name, sizeof(DB_DB_NAME), db_buffer);
    db_buffer[sizeof(DB_DB_NAME)] = 0;
    STtrmwhite(db_buffer);
    db_length = STlength(db_buffer);
    LOingpath(location_buffer, db_buffer, LO_DB, &cl_loc);
    LOtos(&cl_loc, &physical_name);
    physical_length = STlength(physical_name);
    MEmove(physical_length, physical_name, ' ',
	sizeof(dcb.dcb_location.physical), (PTR)&dcb.dcb_location.physical);
    STRUCT_ASSIGN_MACRO(dmm->dmm_db_name, dcb.dcb_name);
    STRUCT_ASSIGN_MACRO(dmm->dmm_owner, dcb.dcb_db_owner);
    dcb.dcb_location.phys_length = STlength(physical_name);
    dcb.dcb_location.flags = 0;
    MEmove(8, "$default", ' ',
	sizeof(dcb.dcb_location.logical), (PTR)&dcb.dcb_location.logical);
    dcb.dcb_access_mode = DCB_A_WRITE;
    dcb.dcb_status = DCB_SINGLE;
    dcb.dcb_db_type = DCB_PUBLIC;
    dcb.dcb_opn_count = 0;
    dcb.dcb_ref_count = 0;

    /*  Check that location exists. */

    status = DIlistfile(&di_context, physical_name, physical_length,
	list_func, 0, &sys_err);
    if (status != DI_ENDFILE)
    {
	SETDBERR(&dmm->error, 0, E_DM011A_ERROR_DB_DIR);
	return (E_DB_ERROR);
    }

    /* Open config file */

    *retval = 0;
    status = dm0c_open(&dcb, 0, xcb->xcb_lk_id, &cnf, &dmm->error);
    if (status == E_DB_OK)
    {
	/* we can be asked to do any of the following:
	**	dmm_flags_mask & DMM_1GET_STATUS - read dsc_status
	**	dmm_flags_mask & DMM_2GET_CMPTLVL - read dsc_dbcmptlvl
	**	dmm_flasg_mask & DMM_3GET_CMPTMIN - read dsc_1dbcmptminor
	**	dmm_flags_mask & DMM_4GET_ACCESS - read dsc_dbaccess
	**	dmm_flags_mask & DMM_5GET_SERVICE - read dsc_db_service
	**	dmm_flags_mask & DMM_UPDATE_CONFIG - update config file
	*/

	if (dmm->dmm_flags_mask & DMM_1GET_STATUS)
	    *retval = dmm->dmm_status = cnf->cnf_dsc->dsc_status;
	else if (dmm->dmm_flags_mask & DMM_2GET_CMPTLVL)
	    *retval = dmm->dmm_cmptlvl = cnf->cnf_dsc->dsc_dbcmptlvl;
	else if (dmm->dmm_flags_mask & DMM_3GET_CMPTMIN)
	    *retval = dmm->dmm_dbcmptminor = cnf->cnf_dsc->dsc_1dbcmptminor;
	else if (dmm->dmm_flags_mask & DMM_4GET_ACCESS)
	    *retval = dmm->dmm_db_access = cnf->cnf_dsc->dsc_dbaccess;
	else if (dmm->dmm_flags_mask & DMM_5GET_SERVICE)
	    *retval = dmm->dmm_db_service = cnf->cnf_dsc->dsc_dbservice;
	else if (dmm->dmm_flags_mask & DMM_UPDATE_CONFIG)
	{   
	    /* you can simultaniously update as many fields as you desire */
	    TRdisplay("DMM_UPDATE_CONFIG upd_map %x \n", dmm->dmm_upd_map);

	    if (dmm->dmm_upd_map & DMM_UPD_STATUS)
		cnf->cnf_dsc->dsc_status = dmm->dmm_status;
	    if (dmm->dmm_upd_map & DMM_UPD_CMPTLVL)
		cnf->cnf_dsc->dsc_dbcmptlvl = dmm->dmm_cmptlvl;
	    if (dmm->dmm_upd_map & DMM_UPD_CMPTMINOR)
		cnf->cnf_dsc->dsc_1dbcmptminor = dmm->dmm_dbcmptminor;
	    if (dmm->dmm_upd_map & DMM_UPD_ACCESS)
		cnf->cnf_dsc->dsc_dbaccess = dmm->dmm_db_access;
	    if (dmm->dmm_upd_map & DMM_UPD_SERVICE)
		cnf->cnf_dsc->dsc_dbservice = dmm->dmm_db_service;
	    if (dmm->dmm_upd_map & DMM_UPD_CVERSION)
		cnf->cnf_dsc->dsc_c_version = dmm->dmm_cversion;
	}

	status = dm0c_close(cnf, 
		    (dmm->dmm_flags_mask & DMM_UPDATE_CONFIG)
			? (DM0C_UPDATE | DM0C_COPY) : 0,
		    &dmm->error);
	if (status == OK)
	    return (OK);
    }
    if (cnf)
    {
	status = dm0c_close(cnf, 0, &local_dberr);
	if ( status && dmm->error.err_code == 0 )
	    dmm->error = local_dberr;
    }
    
    uleFormat(&dmm->error, 0, NULL, ULE_LOG, NULL, 
    		NULL, 0, NULL, &local_error, 0);
    SETDBERR(&dmm->error, 0, E_DM011D_ERROR_DB_ALTER);
    return (E_DB_ERROR);
}
