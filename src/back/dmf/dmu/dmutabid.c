/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
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
#include    <dmacb.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmucb.h>
#include    <dmxcb.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dm2t.h>
#include    <dm1b.h>
#include    <dm1p.h>
#include    <dmse.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dmu.h>
#include    <cm.h>

/**
** Name:  DMUTABID.C - Function used to generate unique table id.
**
** Description:
**      This file contains the functions necessary to generate a unique table
**      id.  Common routines used by all DMU functions are not included here,
**      but are found in DMUCOMMON.C.  This file defines:
**    
**      dmu_tabid()    -  Routine to get generate a unique table id or index
**			  table id (used primarily for storred procedures.)
**
** History:
**      27-Jul-88 (teg)
**	    Created for Roadrunner storred procedures support.
**	6-jul-1992 (bryanp)
**	    Prototyping DMF.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	21-jun-1993 (bryanp)
**	    Correct error handling problems.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	17-sep-96 (nick)
**	    Remove compiler warning.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_DM006A_TRAN_ACCESS_CONFLICT.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-Oct-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**/

/*
** <# defines>
** <typedefs>
** <forward references>
** <externs>
** <statics>
*/


/*{
** Name: dmu_tabid- Creates a unique table identifier.
**
**  INTERNAL DMF call format:    status = dmu_tabid(&dmu_cb);
**
**  EXTERNAL call format:        status = dmf_call(DMU_GET_TABID, &dmu_cb);
**
** Description:
**	The dmu_tabid function handles the generation of a unique table
**	identifier.  This dmu functin is allowd inside a user specified
**	transactin.  The table identifier is comprised of two components:
**	    base table id (db_tab_base)
**	    index table id (db_tab_index)
**	dmu_tabid will always create the table id with a unique base table id
**	and an index table id of zero.
**
** Inputs:
**      dmu_cb 
**         .type                            Must be set to DMU_UTILITY_CB.
**         .length                          Must be at least sizeof(DMU_CB).
**         .dmu_tran_id                     Must be the current transaction 
**                                          identifier returned from the begin 
**                                          transaction operation.
**         .dmu_flags_mask		    Must be zero.
**         .dmu_db_id                       Must be the identifier for the 
**                                          odcb returned from open database.
**
** Outputs:
**         .dmu_tbl_id                      The internal table identifier
**                                          of table created.
**      dmu_cb   
**          .dmu_tbl_id                     The internal table identifier
**                                          assigned to this table.
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM0010_BAD_DB_ID
**                                          E_DM001A_BAD_FLAG
**                                          E_DM0021_TABLES_TOO_MANY
**                                          E_DM002A_BAD_PARAMETER
**                                          E_DM003B_BAD_TRAN_ID
**                                          E_DM0042_DEADLOCK
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**                                          E_DM0065_USER_INTR
**                                          E_DM0064_USER_ABORT
**	    				    E_DM006A_TRAN_ACCESS_CONFLICT
**					    E_DM007B_CANT_CREATE_TABLEID
**                                          E_DM0100_DB_INCONSISTENT:
**                                          E_DM010C_TRAN_ABORTED:
**					    
**         Note the following is returned
**         with E_DB_WARN.
**                                          E_DM0101_SET_JOUNRAL_ON
**
**          .error.err_data                 Set to attribute in error by 
**                                          returning index into attribute 
**                                          array.
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_WARN                       Function completed normally with 
**                                          a termination status which is in 
**                                          dmu_cb.error.err_code.
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dmu_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmu_cb.error.err_code.
**
** History:
**      27-jul-88 (teg)
**          Created for roadrunner.
**	21-jun-1993 (bryanp)
**	    Correct error handling problems.
**	17-sep-96 (nick)
**	    Remove compiler warning.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_DM006A_TRAN_ACCESS_CONFLICT.
**	6-Feb-2004 (schka24)
**	    Get rid of DMU statement count and its limit.
**	    Added parameter to get tabid.
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
*/
DB_STATUS
dmu_tabid(DMU_CB        *dmu_cb)
{
    DMU_CB	    *dmu = dmu_cb;
    DML_XCB	    *xcb;
    DML_ODCB	    *odcb;
    i4         db_lockmode;
    i4	    error, local_error;
    i4	    status;
    DB_TAB_ID	    temp_tbl_id;

    CLRDBERR(&dmu->error);

    status = E_DB_ERROR;
    do
    {
	if (dmu->dmu_flags_mask != 0)
	{
	    SETDBERR(&dmu->error, 0, E_DM001A_BAD_FLAG);
	    break;
	}

	xcb = (DML_XCB*) dmu->dmu_tran_id;
	if (dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) != E_DB_OK)
	{
	    SETDBERR(&dmu->error, 0, E_DM003B_BAD_TRAN_ID);
	    error = E_DM003B_BAD_TRAN_ID;
	    break;
	}

	/* Check for external interrupts */
	if ( xcb->xcb_scb_ptr->scb_ui_state )
	    dmxCheckForInterrupt(xcb, &error);

	if (xcb->xcb_state & XCB_USER_INTR)
	{
	    SETDBERR(&dmu->error, 0, E_DM0065_USER_INTR);
	    break;
	}
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

	odcb = (DML_ODCB*) dmu->dmu_db_id;
	if (dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB) != E_DB_OK)
	{
	    SETDBERR(&dmu->error, 0, E_DM0010_BAD_DB_ID);
	    break;
	}
	if (xcb->xcb_scb_ptr != odcb->odcb_scb_ptr)
	{
	    SETDBERR(&dmu->error, 0, E_DM0010_BAD_DB_ID);
	    break;
	}

	/*  Check that this is a update transaction on the database 
        **  that can be updated. */
	if (odcb != xcb->xcb_odcb_ptr)
	{
	    SETDBERR(&dmu->error, 0, E_DM005D_TABLE_ACCESS_CONFLICT);
	    break;
	}

	if (xcb->xcb_x_type & XCB_RONLY)
	{
	    SETDBERR(&dmu->error, 0, E_DM006A_TRAN_ACCESS_CONFLICT);
	    break;
	}

	db_lockmode = DM2T_S;
	if (odcb->odcb_lk_mode == ODCB_X)
	    db_lockmode = DM2T_X;

	/*
	** call the physical layer to get a unique table identifier from the
	** database configuration file and create a table id from that
	** identifier
	*/

	status = dm2u_get_tabid(odcb->odcb_dcb_ptr, xcb,
				 &temp_tbl_id, FALSE, 1,
				 &dmu->dmu_tbl_id, &dmu->error);

    } while (FALSE);

    if (dmu->error.err_code > E_DM_INTERNAL)
    {
	uleFormat( &dmu->error, 0, NULL, ULE_LOG , NULL, 
	    (char * )NULL, 0L, (i4 *)NULL, 
            &local_error, 0);
	SETDBERR(&dmu->error, 0, E_DM007B_CANT_CREATE_TABLEID);
    }

    if (status != E_DB_OK)
    {
	switch (dmu->error.err_code)
	{
	    case E_DM004B_LOCK_QUOTA_EXCEEDED:
	    case E_DM0112_RESOURCE_QUOTA_EXCEED:
	    case E_DM007B_CANT_CREATE_TABLEID:
	    case E_DM006A_TRAN_ACCESS_CONFLICT:
		xcb->xcb_state |= XCB_STMTABORT;
		break;

	    case E_DM0042_DEADLOCK:
	    case E_DM004A_INTERNAL_ERROR:
	    case E_DM0100_DB_INCONSISTENT:
		xcb->xcb_state |= XCB_TRANABORT;
		break;
	    case E_DM0065_USER_INTR:
		xcb->xcb_state |= XCB_USER_INTR;
		break;
	    case E_DM010C_TRAN_ABORTED:
		xcb->xcb_state |= XCB_FORCE_ABORT;
		break;
	}

	return (status);
    }
    return (E_DB_OK);

}
