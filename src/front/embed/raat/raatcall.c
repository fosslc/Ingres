/* 
** Copyright (c) 2004 Ingres Corporation
*/

# include       <compat.h>
# include       <raat.h>

/*
** Name: raatcall.c - raat call interface
**
** Description:
**	This file defines the Application Programers call interface for
**	simple record at a time actions, these actions will bypass most
**	of the inteligence of the DBMS and make direct calls to the Data
**	Manipulation Facility (DMF). Routines in this file are:
**
**	    IIraat_call - Call interface to RAAT
**
** History:
**	3-apr-95 (stephenb)
**	    First written.
**	16-apr-95 (stephenb)
**	    add calls for record get, record put, record del, record position
**	    record replace and transaction abort.
**	18-apr-95 (lewda02)
**	    Return a status of OK unless failure is detected.  (In many cases,
**	    we do not test for failure.  This must be improved.)
**      8-may-95 (lewda02/thaju02)
**	    Modularized raat operations.
**	    Changed naming from API to RAAT (Record At A Time)
**	18-may-95 (lewda02)
**	    Add case for RAAT_TABLE_LOCK.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** Name: IIraat_call - Call interface to RAAT
**
** Description:
**	This route forms the call interface to the RAAT, all calls to the RAAT
**	should be made through this routine.
**
** Inputs:
**	op_code		RAAT operation code
**	raat_cb		RAAT control block
**
** Outputs:
**	raat_cb		RAAT control block
**
** Returns:
**	STATUS		OK or FAIL
**
*/
STATUS
IIraat_call(i4	op_code, RAAT_CB *raat_cb)
{
    STATUS	status = OK;

    switch (op_code)
    {
	case RAAT_SESS_START:
	    /*
	    ** Start Session
	    */
            status = IIraat_session_start(raat_cb);
	    break;
	case RAAT_SESS_END:
	    /*
	    ** End Session: for now just use the quel disconnect command
	    */
            status = IIraat_session_end(raat_cb);
	    break;
	case RAAT_TX_BEGIN:
	    /*
	    ** Start transaction: for now just use quel begin transaction
	    */
            status = IIraat_tx_begin(raat_cb);
	    break;
	case RAAT_TX_COMMIT:
	    /*
	    ** End Transaction: for now just use quel end transaction
	    */
            status = IIraat_tx_commit(raat_cb);
	    break;
	case RAAT_TX_ABORT:
	    /*
	    ** Abort transaction: for now just use quel
	    */
	    status = IIraat_tx_abort(raat_cb);
	    break;
	case RAAT_TABLE_OPEN:
	    /*
	    ** Open a table
	    */
	    status = IIraat_table_open(raat_cb);
	    break;
	case RAAT_TABLE_LOCK:
	    /*
	    ** Lock a table
	    */
	    status = IIraat_table_lock(raat_cb);
	    break;
	case RAAT_TABLE_CLOSE:
	    /*
	    ** Close a table
	    */
	    status = IIraat_table_close(raat_cb);
	    break;
	case RAAT_RECORD_GET:
	    /*
	    ** Get a record
	    */
	    status = IIraat_record_get(raat_cb);
	    break;
	case RAAT_RECORD_PUT:
	    /*
	    ** put a record
	    */
	    status = IIraat_record_put(raat_cb);
	    break;
	case RAAT_RECORD_POSITION:
	    /*
	    ** position cursor on a record
	    */
	    status = IIraat_record_position(raat_cb);
	    break;
	case RAAT_RECORD_DEL:
	    /*
	    ** delete a record
	    */
	    status = IIraat_record_delete(raat_cb);
	    break;
	case RAAT_RECORD_REPLACE:
	    /*
	    ** replace record at current position
	    */
	    status = IIraat_record_replace(raat_cb);
	    break;
	default:
	    status = FAIL;
	    break;
    }
    return (status);
}
