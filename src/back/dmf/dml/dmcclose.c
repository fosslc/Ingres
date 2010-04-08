/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <cs.h>
#include    <scf.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dm0m.h>
#include    <dm2d.h>
#include    <dm2f.h>
#include    <dm2t.h>

/**
** Name: DMCCLOSE.C - Functions used to close a database for a session.
**
** Description:
**      This file contains the functions necessary to close a database
**      for a session.  This file defines:
**    
**      dmc_close_db() -  Routine to perform the close 
**                        database operation.
**
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.      
**	19-nov-1985 (derek)
**	    Completed code for dmc_close_db().
**	1-Feb-1989 (ac)
**	    Added 2PC support.
**	23-apr-1990 (fred)
**	    Added support for removing any database-open/session level
**	    temporaries.
**	25-mar-1991 (rogerk)
**	    If session ends while in set nologging mode, make an alter
**	    call to turn off the nologging status - and force the database
**	    to a consistent state.
**	8-jul-1992 (walt)
**	    Prototyping DMF.
**	04-nov-92 (jrb)
**	    Deallocate memory for locm_cb when db closes.  For multi-location
**	    sorts project.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system type definitions.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	18-oct-93 (rogerk)
**	    Changes to 2PC protocols.  When a willing commit transaction is
**	    disconnected, we now call dm0l_closedb to remove the database
**	    context from the server.  This is possible now since the xact is
**	    orphaned from our process context in the disconnect_willing_commit
**	    call.  Took out the DM2D_NLG parameter to dm2d_close_db.
**	31-jan-1994 (bryanp) B57930, B57931, B57936, B58650
**	    Log scf_error.err_code if scf_call fails.
**	    Log dm2f_delete_file error unless it's "no such file".
**	    Pass dcb pointer, not odcb pointer, to dm2f_delete_file.
**	    Log dm2t_destroy_temp_tcb error unless it's "no such table".
**      27-dec-1994 (cohmi01)
**          For RAW files, setup additional info in DMP_LOCATION from new
**          fields in XCCB to identify location name and raw extent.
**	22-feb-1995 (cohmi01)
**	    Adjust restoration of raw info from xccb to DMP_LOCATION.
** 	13-jun-1997 (wonst02)
** 	    Bypass dmc_set_logging if readonly database.
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm2t_destroy_temp_tcb() calls.
**	    Added log_id (LG_LXID) to dm2f_build_fcb(), dm2f_open_file(),
**	    dm2f_create_file(), dm2f_delete_file(), dm2f_rename(),
**	    dm2f_add_fcb() dm2f_release_fcb() prototypes so that it 
**	    can be passed on to dm2t_reclaim_tcb().
**	03-Dec-1998 (jenjo02)
**	    Pass XCB, page size, extent start from XCCB 
**	    to dm2f_alloc_rawextent().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	21-mar-1999 (nanpr01)
**	    Deleted obsolete RAW locations code.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Jan-2001 (jenjo02)
**	    Get DML_SCB* from ODCB instead of calling SCF.
**	5-Feb-2004 (schka24)
**	    Toss temp tables out of RDF too.
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	18-Nov-2008 (jonj)
**	    dm2d_? functions converted to DB_ERROR *
**	01-Dec-2008 (jonj)
**	    SIR 120874: dm2f_? functions converted to DB_ERROR *
**/

/*{
** Name: dmc_close_db - Closes a database for a session.
**
**  INTERNAL DMF call format:    status = dmc_close_db(&dmc_cb);
**
**  EXTERNAL call format:        status = dmf_call(DMC_CLOSE_DB,&dmc_cb);
**
** Description:
**    The dmc_close_db function handles the closing of a database for a
**    user session.  All internal DMF control information maintained for
**    accessing a database through the control block identifier by
**    dmc_db_id will be released.  The identifier will no longer be
**    valid.
**
** Inputs:
**      dmc_cb 
**          .type                           Must be set to DMC_CONTROL_CB.
**          .length                         Must be at least sizeof(DMC_CB).
**          .dmc_session_id		    Must be a valid SCF session 
**                                          identifier obtained from SCF.
**          .dmc_op_type                    Must be DMC_DATABASE_OP.
**          .dmc_flag_mask                  Must be zero or DMC_NLK_SESS.
**          .dmc_db_id                      Must be a valid open database 
**                                          identifier returned from an
**                                          open database operation.
**
** Output:
**      dmc_cb 
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM0010_BAD_DB_ID
**                                          E_DM001A_BAD_FLAG
**                                          E_DM002F_BAD_SESSION_ID
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**                                          E_DM0053_NONEXISTENT_DB
**                                          E_DM0060_TRAN_IN_PROGRESS
**                                          E_DM0065_USER_INTR
**                                          E_DM0064_USER_ABORT
**                                          E_DM0087_ERROR_CLOSING_DB
**                                          E_DM0112_RESOURCE_QUOTA_EXCEEDED
**                     
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_WARN                       Function completed normally with 
**                                          a termination status which is in 
**                                          dmc_cb.error.err_code.
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dmc_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmc_cb.error.err_code.
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**	18-nov-1985 (derek)
**	    Completed code.
**	1-Feb-1989 (ac)
**	    Added 2PC support.
**	23-apr-1990 (fred)
**	    Added support for removing any database-open/session level
**	    temporaries.
**	25-mar-1991 (rogerk)
**	    If session ends while in set nologging mode, make an alter
**	    call to turn off the nologging status - and force the database
**	    to a consistent state.
**	8-jul-1992 (walt)
**	    Prototyping DMF.
**	04-nov-92 (jrb)
**	    Deallocate memory for locm_cb when db closes.  For multi-location
**	    sorts project.
**	18-oct-93 (rogerk)
**	    Changes to 2PC protocols.  When a willing commit transaction is
**	    disconnected, we now call dm0l_closedb to remove the database
**	    context from the server.  This is possible now since the xact is
**	    orphaned from our process context in the disconnect_willing_commit
**	    call.  Took out the DM2D_NLG parameter to dm2d_close_db.
**	09-oct-93 (swm)
**	    Bug 56437
**	    Get scf_session_id from new dmc_session_id rather than dmc_id.
**	31-jan-1994 (bryanp) B57930, B57931, B57936, B58650
**	    Log scf_error.err_code if scf_call fails.
**	    Log dm2f_delete_file error unless it's "no such file".
**	    Pass dcb pointer, not odcb pointer, to dm2f_delete_file.
**	    Log dm2t_destroy_temp_tcb error unless it's "no such table".
**      27-dec-1994 (cohmi01)
**          For RAW files, setup additional info in DMP_LOCATION from new
**          fields in XCCB to identify location name and raw extent.
**	22-feb-1995 (cohmi01)
**	    Adjust restoration of raw info from xccb to DMP_LOCATION.
** 	13-jun-1997 (wonst02)
** 	    Bypass dmc_set_logging if readonly database.
**	03-Dec-1998 (jenjo02)
**	    Pass XCB, page size, extent start from XCCB 
**	    to dm2f_alloc_rawextent().
**	08-Jan-2001 (jenjo02)
**	    Get DML_SCB* from ODCB instead of calling SCF.
**	11-Jun-2004 (schka24)
**	    Use new temp delete utility;  release odcb cq mutex.
*/

DB_STATUS
dmc_close_db(
DMC_CB    *dmc_cb)
{
    DMC_CB		*dmc = dmc_cb;
    DML_ODCB		*odcb;
    DML_SCB		*scb;
    DML_XCCB		*xccb;
    i4		error, local_error;
    DB_STATUS		status;
    DB_STATUS		local_status;
    i4		flag = 0;
    DB_ERROR		local_dberr;

    CLRDBERR(&dmc->error);

    for (;;)
    {
	if (dmc->dmc_op_type != DMC_DATABASE_OP)
	{
	    SETDBERR(&dmc->error, 0, E_DM000C_BAD_CB_TYPE);
	    break;
	}

	if ( (odcb = (DML_ODCB *)dmc->dmc_db_id) == 0 ||
	      dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB) ||
	      dm0m_check((DM_OBJECT *)odcb->odcb_db_id, 
                               (i4)DCB_CB) )
	{
	    SETDBERR(&dmc->error, 0, E_DM0010_BAD_DB_ID);
	    break;
	}

	if ( (scb = odcb->odcb_scb_ptr) == 0 ||
	      dm0m_check((DM_OBJECT *)scb, (i4)SCB_CB) ||
	      dm0m_check((DM_OBJECT *)scb->scb_loc_masks,
				(i4)LOCM_CB) )
	{
	    SETDBERR(&dmc->error, 0, E_DM002F_BAD_SESSION_ID);
	    break;
	}

	if (scb->scb_x_ref_count)
	{
	    DML_XCB		*xcb;

	    for (xcb = scb->scb_x_next;
		    xcb != (DML_XCB *)&scb->scb_x_next;
		    xcb = xcb->xcb_q_next)
	    {
		if (xcb->xcb_state & XCB_WILLING_COMMIT)
		{
		    xcb->xcb_state |= XCB_DISCONNECT_WILLING_COMMIT;
		    scb->scb_state |= SCB_WILLING_COMMIT;
		}
		else
		    break;
	    }
	    SETDBERR(&dmc->error, 0, E_DM0060_TRAN_IN_PROGRESS);
	    break;
	}

        /*
        ** If this session is running in Set Nologging mode, then
        ** turn logging back on (even though we are not going to log
        ** anything) to force the database to a consistent state.
        */
        if (scb->scb_sess_mask & SCB_NOLOGGING &&
	    ((DMP_DCB *)odcb->odcb_db_id)->dcb_access_mode != DCB_A_READ)
        {
            status = dmc_set_logging(dmc, scb, (DMP_DCB *)odcb->odcb_db_id,
                			DMC_C_ON);
            if (status != E_DB_OK)
            {
                uleFormat(&dmc->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
		    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &error, 0);
		SETDBERR(&dmc->error, 0, E_DM0087_ERROR_CLOSING_DB);
                break;
            }
        }

	/* 
        ** See if there are any files pended for delete, or any
        ** temporary tables pended for destroy.
	** Shouldn't have parallel threads at this point, but lock
	** anyway just in case.
        */

	dm0s_mlock(&odcb->odcb_cq_mutex);
	while (odcb->odcb_cq_next != (DML_XCCB*) &odcb->odcb_cq_next)
	{
	    /* Get pend XCCB */

	    xccb = odcb->odcb_cq_next;
	    xccb->xccb_q_prev->xccb_q_next = xccb->xccb_q_next;
	    xccb->xccb_q_next->xccb_q_prev = xccb->xccb_q_prev;
	    dm0s_munlock(&odcb->odcb_cq_mutex);
	    if (xccb->xccb_operation == XCCB_F_DELETE)
	    {
		DI_IO		    di_file;
		i4		    filelength;
		DMP_LOCATION        loc;
		i4             loc_count = 1;
		DMP_LOC_ENTRY       ext;
		DMP_FCB             fcb;
		
		MEcopy((char *)&xccb->xccb_p_name, 
		    sizeof(DM_PATH), (char *)&ext.physical);
		ext.phys_length = xccb->xccb_p_len;
		MEcopy((char *)&xccb->xccb_f_name, 
		    sizeof(DM_FILE), (char *)&fcb.fcb_filename);
		fcb.fcb_namelength = xccb->xccb_f_len;
		fcb.fcb_di_ptr = &di_file;
		fcb. fcb_last_page = 0;
		fcb.fcb_location = (DMP_LOC_ENTRY *) 0;
		fcb.fcb_state = FCB_CLOSED;
		loc.loc_ext = &ext;
		loc.loc_fcb = &fcb;
		/* Location id not important since file name is 
		** xccb. Set to zero. */
		loc.loc_id = 0;
		loc.loc_status = 0;    /* not really a valid loc */
		local_status = dm2f_delete_file(scb->scb_lock_list, 
				    xccb->xccb_xcb_ptr->xcb_log_id, &loc,
				    loc_count, (DM_OBJECT *)odcb->odcb_dcb_ptr,
				    (i4)0,
				    &local_dberr);
		if (local_status)
		{
		    uleFormat( &local_dberr, 0, NULL, ULE_LOG , NULL, 
			    (char * )NULL, 0L, (i4 *)NULL, &local_error, 0);
		    uleFormat( NULL, E_DM0087_ERROR_CLOSING_DB,
			    NULL, ULE_LOG , NULL, 
			    (char * )NULL, 0L, (i4 *)NULL, &local_error, 0);
		}

	    }
	    if (xccb->xccb_operation == XCCB_T_DESTROY)
	    {
		DB_ERROR local_dberror;

		local_status = dmt_destroy_temp(scb, xccb, &local_dberror);

		/*
		** It's fine if the temporary table is already gone; any other
		** error, however, should be logged.
		*/
		if (local_status != E_DB_OK &&
		    local_dberror.err_code != E_DM0054_NONEXISTENT_TABLE)
		{
		    uleFormat( &local_dberror, 0, NULL, ULE_LOG , NULL, 
			    (char * )NULL, 0L, (i4 *)NULL, &local_error, 0);
		    uleFormat( NULL, E_DM0095_ERROR_COMMITING_TRAN,
			    NULL, ULE_LOG , NULL, 
			    (char * )NULL, 0L, (i4 *)NULL, &local_error, 0);
		}
	    }
	    dm0m_deallocate((DM_OBJECT **)&xccb);
	    dm0s_mlock(&odcb->odcb_cq_mutex);
	} /* end while */
	dm0s_munlock(&odcb->odcb_cq_mutex);

	/*
	** If Two Phase Commit disconnect protocol then pass the NLK_SESS
	** request to the close_db call.  This causes us to not release the
	** database lock which must remain held by the non-resolved willing
	** commit transaction.  The lock will be orphaned along with the 
	** transaction and all its other locks later when the session is ended,
	** waiting to be adopted later by a 2PC reconnect session.
	*/
	if ((dmc->dmc_flags_mask & DMC_NLK_SESS) ||
	    (scb->scb_state & SCB_WILLING_COMMIT))
	{
	    flag |= DM2D_NLK_SESS;
	}

	status = dm2d_close_db(odcb->odcb_dcb_ptr, scb->scb_lock_list, 
							flag, &dmc->error);
	if (status != E_DB_OK)
	{
	    if (dmc->error.err_code > E_DM_INTERNAL)
	    {
		uleFormat( &dmc->error, 0, NULL, ULE_LOG , NULL, (char * )NULL,
				0L, (i4 *)NULL, &local_error, 0);
		SETDBERR(&dmc->error, 0, E_DM0087_ERROR_CLOSING_DB);
	    }
	    break;
	}

	scb->scb_o_ref_count--;
	odcb->odcb_q_next->odcb_q_prev = odcb->odcb_q_prev;
	odcb->odcb_q_prev->odcb_q_next = odcb->odcb_q_next;
	dm0s_mrelease(&odcb->odcb_cq_mutex);
	dm0m_deallocate((DM_OBJECT **)&odcb);

	if (scb->scb_loc_masks != NULL)
	{
	    dm0m_deallocate((DM_OBJECT **)&scb->scb_loc_masks);
	    scb->scb_loc_masks = NULL;
	}

	return (E_DB_OK);
    }

    return (E_DB_ERROR);
}
