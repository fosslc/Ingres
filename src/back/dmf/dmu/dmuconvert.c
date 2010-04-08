/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <pc.h>
#include    <sr.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmucb.h>
#include    <dmxcb.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dmxe.h>
#include    <dm2t.h>
#include    <dm1b.h>
#include    <dmpp.h>
#include    <dm1p.h>
#include    <di.h>
#include    <dmse.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dmftrace.h>
#include    <dmu.h>

/**
**
** Name: DMUCONVERT.C - Functions needed to convert a table to a new format
**
** Description:
**	This file contains the functions necessary to convert a table
**	to a new format. 
**    
**      dmu_convert()       -  Routine to convert a table.
**
** History:
**      15-October-1992 (rmuth) 
**	    Created for the file extend conversion code.
**	18-dec-1992 (robf)
**          Removed obsolete dm0a.h
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_DM006A_TRAN_ACCESS_CONFLICT.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      22-jan-2007 (stial01)
**          Always break after dm2u_convert (b117528) 
**	23-Oct-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**/

/*{
** Name: dmu_convert -
**
**  INTERNAL DMF call format:      status = dmu_convert(&dmu_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMU_CONVERT_TABLE,&dmu_cb);
**
** Description:
**      The dmu_modify function handles the modification of the 
**
** Inputs:
**      dmu_cb 
**          .type                           Must be set to DMU_UTILITY_CB.
**          .length                         Must be at least sizeof(DMU_CB).
**	    .dmu_flags_mask		    Must be zero.
**          .dmu_tran_id                    Must be the current transaction 
**                                          identifier returned from the 
**                                          begin transaction operation.
**          .dmu_tbl_id                     Internal name of table to be 
**                                          modified.
**          .dmu_location.data_address      Pointer to array of locations.
**                                          Each entry in array is of type
**                                          DB_LOC_NAME.  Must be zero if
**                                          no locations specified.
**	    .dmu_location.data_in_size      The size of the location array
**                                          in bytes.
**          .dmu_key_array.ptr_address	    Pointer to array of pointer. Each
**                                          Pointer points to an attribute
**                                          entry of type DMU_KEY_ENTRY.  See
**                                          below for description of entry.
** Output:
**      dmu_cb
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**					    E_DM001A_BAD_FLAG
**                                          E_DM0064_USER_INTR
**                                          E_DM010C_TRAN_ABORTED
**                                          E_DM0065_USER_ABORT
**                                          E_DM0010_BAD_DB_ID
**                                          E_DM009F_ILLEGAL_OPERATION
**                                          E_DM005E_CANT_UPDATE_SYSCAT
**					    E_DM0137_GATEWAY_ACCESS_ERROR
**					    E_DM00A1_ERROR_CONVERTING_TBL
**	    				    E_DM006A_TRAN_ACCESS_CONFLICT
**
** History:
**      30-October-1992 (rmuth) 
**          Created.
**	18-dec-1992 (robf)
**          Removed obsolete dm0a.h
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_DM006A_TRAN_ACCESS_CONFLICT.
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
**      22-jan-2007 (stial01)
**          Always break after dm2u_convert (b117528) 
*/

DB_STATUS
dmu_convert(dmu_cb)
DMU_CB    *dmu_cb;
{
    DMU_CB		*dmu = dmu_cb;
    DML_XCB		*xcb;
    DML_ODCB		*odcb;
    DMU_CHAR_ENTRY	*chr;
    DMU_KEY_ENTRY	**key;
    i4		chr_count;
    i4		error, local_error;
    DB_STATUS		status;
    i4             db_lockmode;
    DB_LOC_NAME         *location;
    i4             loc_count= 0;
    bool                bad_loc;
    i4		mask;
    DB_OWN_NAME		table_owner;
    DB_TAB_NAME		table_name;

    CLRDBERR(&dmu->error);

    for (status = E_DB_ERROR;;)
    {
	/*  
	** Check for bad flags. 
	*/
	mask = ~(DMU_VGRANT_OK);
        if ( (dmu->dmu_flags_mask & mask) != 0)
	{
	    SETDBERR(&dmu->error, 0, E_DM001A_BAD_FLAG);
	    break;
	}

	/*  
	** Validate the transaction id. 
	*/
	xcb = (DML_XCB *)dmu->dmu_tran_id;
	if (dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) != E_DB_OK)
	{
	    SETDBERR(&dmu->error, 0, E_DM003B_BAD_TRAN_ID);
	    break;
	}

	/* Check for external interrupts */
	if ( xcb->xcb_scb_ptr->scb_ui_state )
	    dmxCheckForInterrupt(xcb, &error);

	/*
	** Check if we are aborting the xact for any reason
	*/
	if ( xcb->xcb_state )
	{
	    if (xcb->xcb_state & XCB_USER_INTR)
	    {
		SETDBERR(&dmu->error, 0, E_DM0065_USER_INTR);
		break;
	    }
	    if (xcb->xcb_state & XCB_FORCE_ABORT)
	    {
		SETDBERR(&dmu->error, 0, E_DM010C_TRAN_ABORTED);
		break;
	    }
	    if (xcb->xcb_state & XCB_ABORT)
	    {
		SETDBERR(&dmu->error, 0, E_DM0064_USER_ABORT);
		break;
	    }	    
	}

	/*  
	** Check the database identifier. 
	*/
	odcb = (DML_ODCB *)dmu->dmu_db_id;
	if (dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB) != E_DB_OK)
	{
	    SETDBERR(&dmu->error, 0, E_DM0010_BAD_DB_ID);
	    break;
	}

	/*  
	** Check that this is a update transaction on the database 
        ** that can be updated. 
	*/
	if (odcb != xcb->xcb_odcb_ptr)
	{
	    SETDBERR(&dmu->error, 0, E_DM005D_TABLE_ACCESS_CONFLICT);
	    break;
	}

	if (xcb->xcb_x_type & XCB_RONLY)
	{
	    SETDBERR(&dmu->error, 0, E_DM006A_TRAN_ACCESS_CONFLICT);
	    xcb->xcb_state |= XCB_STMTABORT;
	    break;
	}

    
        /* 
	** Calls the physical layer to process the rest of the convert
	*/
	status = dm2u_convert(
			odcb->odcb_dcb_ptr, 
			xcb,
			&dmu->dmu_tbl_id, 
			&dmu->error);

	if (status != E_DB_OK)
	{
	    xcb->xcb_state |= XCB_STMTABORT;
	    break;
	}

	break; /* always */

    }

    if ( status == E_DB_OK )
	return( E_DB_OK );


    /*
    ** Error condtion so perform the following
    **	- Write the lower level error to the error log file
    **  - Write an error message describing the dbname and table_name
    **	  to the error log file
    **  - Pass back the conversion error message to the 
    **  
    */

    uleFormat(&dmu->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
    		(char * )NULL, (i4)0, (i4 *)NULL, &local_error, 0);

    uleFormat( NULL, E_DM92CC_ERR_CONVERT_TBL_INFO, NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 3,
		0, dmu->dmu_tbl_id.db_tab_base,
		0, dmu->dmu_tbl_id.db_tab_index,
		sizeof(odcb->odcb_dcb_ptr->dcb_name),
		&odcb->odcb_dcb_ptr->dcb_name);

    SETDBERR(&dmu->error, 0, E_DM00A1_ERROR_CONVERTING_TBL);

    return (E_DB_ERROR);
}
