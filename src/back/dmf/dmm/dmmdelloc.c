/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <pc.h>
#include    <ck.h>
#include    <cs.h>
#include    <cv.h>
#include    <di.h>
#include    <jf.h>
#include    <lo.h>
#include    <me.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <lk.h>
#include    <lg.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dmccb.h>
#include    <dmmcb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm2d.h>
#include    <dm0c.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dmm.h>

/**
**
** Name: DMMDELLOC.C - Functions needed to delete location from database config file.
**
** Description:
**      This file contains the functions necessary to delete locations from
**	the database configuration file. This function is used by the INGCNTRL
**	utility to unextend a database to allow files to be removed from a
**	location.
**    
**      dmm_del_location()       -  Routine to perfrom the delete operation.
**
** History:
**      21-apr-2004 (gorvi01) SRS Section 3.1.27 Unextenddb
**          Created for Unextenddb.
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	05-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	18-Nov-2008 (jonj)
**	    dm2d_? functions converted to DB_ERROR *
**/

/*
** Forward Declarations
*/


/*{
** Name: dmm_del_location - Verifies/makes Area path and/or deletes a 
**			    location from a database.
**
**  INTERNAL DMF call format:      status = dmm_del_loc(&dmm_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMM_DEL_LOCATION, &dmm_cb);
**
** Description:
**	Depending on one or more flags in dmm_flags_mask:
**
**
** Inputs:
**      dmm_cb 
**          .type                           Must be set to DMM_MAINT_CB.
**          .length                         Must be at least sizeof(DMM_CB).
**          .dmm_tran_id                    Must be the current transaction 
**                                          identifier returned from the 
**                                          begin transaction operation.
**          .dmm_location.data_address      Pointer to array of locations.
**                                          Each entry in array is of type
**                                          DB_LOC_NAME.  Must be zero if
**                                          no locations specified.
**	    .dmm_location.data_in_size      The size of the location array
**                                          in bytes.
**	    dmm_loc_type		    Type of location:
**						DMM_L_JNL
**						DMM_L_CKP
**						DMM_L_DMP
**						DMM_L_WRK
**						DMM_L_AWRK
**						DMM_L_DATA
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
**	21-apr-2004 (gorvi01)
**	    Created for Unextenddb utility.
**	10-Sep-2004 (gorvi01)
**          Added MEfill for db_dir to avoid any memory corruption of
**          location name that is used later. Fixes bug 113004.
**	28-Dec-2004 (jenjo02)
**	    Cleaned up sporadic tabs which made the code unreadable.
**	    Use DMM_CHECK_LOC_AREA to validate the location yet
**	    keep it from being created if it doesn't exist.
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
**	09-Mar-2009 (drivi01)
**	    Initialize l_db_dir, l_area, loc_flags and physical_length
**	    variables to avoid bogus values.
*/
DB_STATUS 
dmm_del_location(
    DMM_CB *dmm_cb)
{
    DMM_CB	    *dmm = dmm_cb;
    DML_XCB	    *xcb;
    DML_SCB	    *scb;
    STATUS	    status;
    i4		    *err_code = &dmm->error.err_code;
    i4	    	    l_db_dir = 0;
    i4	    	    l_area = 0;
    i4	    	    loc_flags = 0;
    char	    *physical_name;
    i4	    	    physical_length = 0;
    char	    db_buffer[sizeof(DB_DB_NAME) + sizeof(int)];
    char	    db_dir[sizeof(DM_PATH) + sizeof(int)];
    char	    area[sizeof(DM_PATH) + sizeof(int)];
    char	    location_buffer[132];
    LOCATION	    cl_loc;
    DMM_LOC_LIST    **loc_list;

    CLRDBERR(&dmm->error);

    /*	Check arguments. */

    for (;;)
    {
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

	/*  Check Database location. */

	if ( dmm->error.err_code == 0 )
	{
	    if (dmm->dmm_db_location.data_in_size == 0 ||
		    dmm->dmm_db_location.data_address == 0 ||
		    (dmm->dmm_loc_list.ptr_in_count &&
		    (dmm->dmm_loc_list.ptr_size != sizeof(DMM_LOC_LIST) ||
		    dmm->dmm_loc_list.ptr_address == 0 ||
		    dmm->dmm_loc_list.ptr_in_count != 1)))
	    {
		SETDBERR(&dmm->error, 0, E_DM0072_NO_LOCATION);
	    }
	}

	if ( dmm->error.err_code )
	{
	    status = E_DB_ERROR;
	    break;
	}

	/*  Check location to be deleted. */

	loc_list = (DMM_LOC_LIST **)dmm->dmm_loc_list.ptr_address;
	loc_flags = DMM_CHECK_LOC_AREA;

	MEfill(sizeof(db_dir),'\0',db_dir);

	/*
	** We don't really care if the location's path exists or not,
	** but we'll call dmm_loc_validate to verify that all 
	** the pieces fit together. Note that because we specify
	** DMM_CHECK_LOC_AREA, the "db_dir" that's constructed
	** is -not- appended with the DB name, but we don't
	** care; dm2d_unextend_db will use the location's 
	** physical path found in the config file.
	*/
	if ( status = dmm_loc_validate(loc_list[0]->loc_type, 
			&dmm->dmm_db_name, 
			loc_list[0]->loc_l_area, loc_list[0]->loc_area,
			area, &l_area, db_dir, &l_db_dir, &loc_flags, &dmm->error) )
	{
	    if ( dmm->error.err_code == E_DM0080_BAD_LOCATION_AREA )
	    {
		CLRDBERR(&dmm->error);
		status = E_DB_OK;
	    }
	}

	if ( status == E_DB_OK )
	{
	    /*
	    ** Make path out of root DB location information,
	    ** i.e., where to look for CONFIG file.
	    */
	    MEcopy(dmm->dmm_db_location.data_address,
		    dmm->dmm_db_location.data_in_size,
		    location_buffer);
		    location_buffer[dmm->dmm_db_location.data_in_size] = 0;
	    MEcopy((char *)&dmm->dmm_db_name, sizeof(dmm->dmm_db_name), db_buffer);
	    db_buffer[sizeof(dmm->dmm_db_name)] = 0;
	    STtrmwhite(db_buffer);

	    if ( LOingpath(location_buffer, db_buffer, LO_DB, &cl_loc) )
	    {
		SETDBERR(&dmm->error, 0, E_DM0080_BAD_LOCATION_AREA);
		status = E_DB_ERROR;
	    }
	    else
	    {
		LOtos(&cl_loc, &physical_name);

		physical_length = STlength(physical_name);

		/* Remove location info from config file */
		status = dm2d_unextend_db(scb->scb_lock_list, &dmm->dmm_db_name,
			physical_name, physical_length, &loc_list[0]->loc_name,
			area, l_area, db_dir, l_db_dir, loc_flags, 
			loc_list[0]->loc_raw_pct,
			&loc_list[0]->loc_raw_start, 
			&loc_list[0]->loc_raw_blocks, 
			&dmm->error);
	    }
	}
	break;
    }
    return (status);    
}
