/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <cs.h>
#include    <adf.h>
#include    <ade.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <gwf.h>
#include    <gwfint.h>

/**
** Name: GWFREC.C - generic gateway record-level functions.
**
** Description:
**	This file contains the record-level functions for the GWF facility.
**
**      This file defines the following external functions:
**    
**	gwr_position() - GW record stream positioning
**	gwr_get() - GW record stream get
**	gwr_put() - GW record put
**	gwr_replace() - GW record replace
**	gwr_delete() - GW record delete
**
** History:
**	04-apr-90 (linda)
**	    Created -- broken out from gwf.c which is now obsolete.
**	18-apr-90 (bryanp)
**	    Re-map exit-level error codes to GWF-level error codes where
**	    appropriate. The GWF-level error codes then get remapped to
**	    DMF-level error codes by DMF.
**	11-jun-1990 (bryanp)
**	    Re-map E_GW5484_RMS_DATA_CVT_ERROR to E_GW050D_DATA_CVT_ERROR.
**	14-jun-1990 (linda, bryanp)
**	    Added support for GWT_OPEN_NOACCESS.
**	18-jun-1990 (linda, bryanp)
**	    Added support for GWT_MODIFY.
**	4-feb-91 (linda)
**	    Code cleanup:  reflect new structure member names; corrections to
**	    silence unix compiler warnings.
**	8-aug-91 (rickh)
**	    If an error has already been reported to the user, spare us the
**	    alarming yet uninformative error-babble of higher facilities.
**      18-may-92 (schang)
**          GW merge.  Roll in 6.4 improvement
**	25-sep-92 (robf)
**	    Pass gw_id to exit routine in case it checks it.
**      06-nov-92 (schang)
**          assign access_mode  so that exit routines can test the real thing.
**      12-apr-93 (schang)
**          add interrupt support
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      23-aug-93 (schang)
**          in gwr_get, retry record get if error indicates repeating group
**          out of data.
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
*/

/*{
** Name: gwr_position - GW record stream position
**
** Description:
**
**	This function defines or redefines a GW table record stream position.
**	This request must precede other record stream operations, except
**	gwr_put.  A gwt_open() request must have preceded this request.  A
**	range of key values can be defined if the table is keyed or if it was
**	opened with idx_id set (i.e. it's a secondary index);  else it must be
**	a complete scan.  Optional qualifications can be specified for further
**	restricting the record stream members.
**
**	A keyed table or one opened with idx_id set requires key range
**	specification.  Note that for exact key match, low_key should be made
**	equal to high_key.
**
**	An optional qualification function and arguments can be specified.
**	gwr_get only returns records that satisfy the qualification.
**
**	The record stream qualification function and arguments only affect the
**	record get operations, and they are expected to be immutable until
**	another position or close request on the record stream.
**
**	The qualification function is expected to be applied with two
**	arguments:  one is the given gualification argument and the other the
**	record buffer address.  The function is expected to return non-zero if
**	the record is qualified, else it returns zero.
**
** Inputs:
**	gw_rcb->
**		tab_id		the ID of the table to position.
**		in_data1	lower key of the index range; null 
**				indicates beginning-of-file positioning
**		in_data2	higher key of the index range; null 
**				indicates end-of-file positioning
**		qual_func	qualification function
**		qual_arg	qualification argument
**		gw_rsb		table environment returned by gwt_open
**
** Output:
**	gw_rcb->
**		error.err_code  One of the following error numbers.
**				E_GW020A_GWR_POSITION_ERROR
**				E_GW0504_LOCK_QUOTA_EXCEEDED
**				E_GW0507_FILE_UNAVAILABLE
**				E_GW0508_DEADLOCK
**				E_GW0509_LOCK_TIMER_EXPIRED
**				E_GW0600_NO_MEM
**				E_GW0641_END_OF_STREAM
**                              E_GW0000_OK                
**				E_GW050E_ALREADY_REPORTED
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
** History:
**      24-Apr-1989 (alexh)
**          Created.
**	26-dec-89 (paul)
**	    Updated to be closer to working.
**	18-apr-90 (bryanp)
**	    Added re-mapping of exit-level error codes to GWF-level error
**	    codes. Also updated to server conventions (single point of return).
**	14-jun-1990 (linda, bryanp)
**	    Added support for GWT_OPEN_NOACCESS.
**	15-jul-90 (linda)
**	    Copy access mode to gwx_rcb before calling exit routine.
**	8-aug-91 (rickh)
**	    If an error has already been reported to the user, spare us the
**	    alarming yet uninformative error-babble of higher facilities.
**	25-sep-92 (robf)
**	    Pass gw_id to exit routine in case it checks it.
**	7-oct-92 (daveb)
**	    pass gw_session to exit in case it wants to add an SCB.
**	    prototyped.  removed dead variable "tcb"
**      05-apr-99 -- 24-jun-96 (schang) 6.4 integration
**        15-mar-94 (schang)
**            add to gwr_position
**            gwx_rcb.xrcb_xhandle =
**             Gwf_facility->gwf_gw_info[rsb->gwrsb_tcb->gwt_gw_id].gwf_xhandle;
**            gwx_rcb.xrcb_xbitset =
**             Gwf_facility->gwf_gw_info[rsb->gwrsb_tcb->gwt_gw_id].gwf_xbitset;
**	14-Apr-2008 (kschendel)
**	    Update qualification mechanism to match DMF's.
*/
DB_STATUS
gwr_position(GW_RCB	*gw_rcb)
{
    GW_SESSION	*session = (GW_SESSION *)gw_rcb->gwr_session_id;
    GW_RSB	*rsb = (GW_RSB *)gw_rcb->gwr_gw_rsb;
    GWX_RCB	gwx_rcb;
    DB_STATUS	status = E_DB_OK;

    for (;;)
    {
	if ((gw_rcb->gwr_gw_id <= 0) || (gw_rcb->gwr_gw_id >= GW_GW_COUNT) ||
	    (Gwf_facility->gwf_gw_info[gw_rcb->gwr_gw_id].gwf_gw_exist == 0))
	{
	    gwf_error( E_GW0400_BAD_GWID, GWF_INTERR, 1,
			sizeof( gw_rcb->gwr_gw_id ), &gw_rcb->gwr_gw_id );
	    gw_rcb->gwr_error.err_code = E_GW020A_GWR_POSITION_ERROR;
	    status = E_DB_ERROR;
	    break;
	}

	switch (rsb->gwrsb_access_mode)
	{
	    case    GWT_READ:
	    case    GWT_WRITE:
			break;
	    case    GWT_OPEN_NOACCESS:
	    case    GWT_MODIFY:
	    default:
		    gwf_error( E_GW0402_RECORD_ACCESS_CONFLICT, GWF_INTERR,
				0);
		    gw_rcb->gwr_error.err_code = E_GW020A_GWR_POSITION_ERROR;
		    status = E_DB_ERROR;
		    break;
	}

	if (status != E_DB_OK)
	    break;
    
	rsb->gwrsb_qual_func = gw_rcb->gwr_qual_func;
	rsb->gwrsb_qual_arg = gw_rcb->gwr_qual_arg;
	rsb->gwrsb_qual_rowaddr = gw_rcb->gwr_qual_rowaddr;
	rsb->gwrsb_qual_retval = gw_rcb->gwr_qual_retval;
	STRUCT_ASSIGN_MACRO(gw_rcb->gwr_in_vdata1, gwx_rcb.xrcb_var_data1);
	STRUCT_ASSIGN_MACRO(gw_rcb->gwr_in_vdata2, gwx_rcb.xrcb_var_data2);
	STRUCT_ASSIGN_MACRO(gw_rcb->gwr_in_vdata_lst,
			    gwx_rcb.xrcb_var_data_list);
	gwx_rcb.xrcb_rsb = (PTR)rsb;
	gwx_rcb.xrcb_access_mode = rsb->gwrsb_access_mode;
	gwx_rcb.xrcb_tab_id = &gw_rcb->gwr_tab_id;
	gwx_rcb.xrcb_gw_id = rsb->gwrsb_tcb->gwt_gw_id;
	gwx_rcb.xrcb_gw_session = session;

        /* 
        **  schang : pass gateway-specific memory to gateway exit
        */
        gwx_rcb.xrcb_xhandle =
        Gwf_facility->gwf_gw_info[rsb->gwrsb_tcb->gwt_gw_id].gwf_xhandle;
        gwx_rcb.xrcb_xbitset =
        Gwf_facility->gwf_gw_info[rsb->gwrsb_tcb->gwt_gw_id].gwf_xbitset;

	if ((status =
	    (*Gwf_facility->gwf_gw_info[rsb->gwrsb_tcb->gwt_gw_id].gwf_gw_exits
					    [GWX_VPOSITION])
	    (&gwx_rcb)) != E_DB_OK)
	{
	    /*
	    ** The position exit failed. There are many possible reasons for
	    ** this, and some of them can be mapped into error-codes which
	    ** DMF 'knows' about and can handle sensibly. As part of this
	    ** process, we now map exit-specific errors into generic GWF
	    ** errors, some of which will be remapped into DMF errors on the
	    ** next level up.
	    */
	    switch (gwx_rcb.xrcb_error.err_code)
	    {
		case E_GW540D_RMS_DEADLOCK:
		    _VOID_ gwf_error( gwx_rcb.xrcb_error.err_code, GWF_INTERR,
				      0 );
		    gw_rcb->gwr_error.err_code = E_GW0508_DEADLOCK;
		    break;

		case E_GW5401_RMS_FILE_ACT_ERROR:
		    _VOID_ gwf_error( gwx_rcb.xrcb_error.err_code, GWF_INTERR,
				      0 );
		    gw_rcb->gwr_error.err_code = E_GW0507_FILE_UNAVAILABLE;
		    break;

		case E_GW5404_RMS_OUT_OF_MEMORY:
		    _VOID_ gwf_error( gwx_rcb.xrcb_error.err_code, GWF_INTERR,
				      0 );
		    gw_rcb->gwr_error.err_code = E_GW0600_NO_MEM;
		    break;

		case E_GW540C_RMS_ENQUEUE_LIMIT:
		    _VOID_ gwf_error( gwx_rcb.xrcb_error.err_code, GWF_INTERR,
				      0 );
		    gw_rcb->gwr_error.err_code = E_GW0504_LOCK_QUOTA_EXCEEDED;
		    break;

		case E_GW5414_RMS_TIMED_OUT:
		    _VOID_ gwf_error( gwx_rcb.xrcb_error.err_code, GWF_INTERR,
				      0 );
		    gw_rcb->gwr_error.err_code = E_GW0509_LOCK_TIMER_EXPIRED;
		    break;

		case E_GW5411_RMS_RECORD_NOT_FOUND:
		case E_GW0641_END_OF_STREAM:
		    gw_rcb->gwr_error.err_code = E_GW0641_END_OF_STREAM;
		    break;

		case E_GW050E_ALREADY_REPORTED:
		case E_GW0023_USER_ERROR:
		    /* Just leave it alone */
		    gw_rcb->gwr_error = gwx_rcb.xrcb_error;
		    break;

		/*
		** These will probably need handling, but at the current time
		** I'm not really sure what to map them to. For now, we'll just
		** have to turn them into unknown error.  Some of these are
		** almost NOT errors (READ_AFTER_WAIT), and so will result in
		** setting STATUS back to OK. Others are errors which should be
		** re-mapped.
		*/
		case E_GW540E_RMS_ALREADY_LOCKED:
		case E_GW540F_RMS_PREVIOUSLY_DELETED:
		case E_GW5410_RMS_READ_LOCKED_RECORD:
		case E_GW5412_RMS_READ_AFTER_WAIT:
		case E_GW5413_RMS_RECORD_LOCKED:
		default:
		    _VOID_ gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR,
				     0);
		    gw_rcb->gwr_error.err_code = E_GW020A_GWR_POSITION_ERROR;
		    break;
	    }
	    break;
	}

	/*
	** Successful end of gwr_position:
	*/
	break;
    }

    if (status == E_DB_OK)
	gw_rcb->gwr_error.err_code = E_DB_OK;

    return(status);
}

/*{
** Name: gwr_get - GW record stream get
**
** Description:
**	This function performs a record get from a positioned record stream.
**	gwr_get() always gets the next record.  Parameter gw_rsb must   have
**	been positioned by a gwr_position() request.
**
** Inputs:
**	gw_rcb->
**	  gw_rsb		positioned record stream id
**	  out_vdata1.
**	    data_address	address of the tuple buffer
**	    data_in_size	the buffer size
**
** Output:
**	gw_rcb->
**	  out_vdata1.
**	    data_address	record stored at this address
**	    data_in_size	size of the record returned
**	  error.err_code  	One of the following error numbers.
**				E_GW020B_GWR_GET_ERROR
**				E_GW0504_LOCK_QUOTA_EXCEEDED
**				E_GW0507_FILE_UNAVAILABLE
**				E_GW0508_DEADLOCK
**				E_GW0509_LOCK_TIMER_EXPIRED
**				E_GW050D_DATA_CVT_ERROR
**				E_GW0600_NO_MEM
**				E_GW0641_END_OF_STREAM
**				E_GW050E_ALREADY_REPORTED
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
** History:
**      24-Apr-1989 (alexh)
**          Created.
**	26-dec-89 (paul)
**	    New code that works.
**	18-apr-90 (bryanp)
**	    Re-map exit-level errors into generic GWF-level errors where
**	    appropriate in order to return useful information to DMF.
**	11-jun-1990 (bryanp)
**	    Re-map E_GW5484_RMS_DATA_CVT_ERROR to E_GW050D_DATA_CVT_ERROR.
**	14-jun-1990 (linda, bryanp)
**	    Added support for GWT_OPEN_NOACCESS.
**	4-feb-1991 (linda)
**	    Support for gateway secondary index access:  see if we're in a
**	    restricted tid join, and process accordingly.
**	30-apr-1991 (rickh)
**	    Removed 4-feb-1991 logic.  Dis-integrated tidjoin support.
**	14-jun_1991 (rickh)
**	    Changed name of GWX_BYPOS to GWX_GETNEXT and GWR_BYPOS to
**	    GWR_GETNEXT since that's how they're used.
**	8-aug-91 (rickh)
**	    If an error has already been reported to the user, spare us the
**	    alarming yet uninformative error-babble of higher facilities.
**	25-sep-92 (robf)
**	    Pass gw_id to exit routine in case it checks it.
**	7-oct-92 (daveb)
**	    pass gw_session to exit in case it wants to add an SCB.
**	    Prototyped.
**      06-nov-92 (schang)
**          assign access_mode  so that exit routines can test the real thing.
**      12-apr-93 (schang)
**          add interrupt support
**      26-may-93 (schang)
**          add support for repeating group
**	16-jun-93 (robf)
**	    Pass table name to exit in case  it uses it (e.g. for diagnostics)
**      24-jun-96 (schang) integrate 64 changes
**        12-aug-93 (schang)
**          pass to exit xbitset
**	14-Apr-2008 (kschendel)
**	    Revise qualification function call to match new DMF style.
*/
DB_STATUS
gwr_get(GW_RCB	*gw_rcb)
{
    GW_SESSION	    *session = (GW_SESSION *)gw_rcb->gwr_session_id;
    DB_STATUS	    status = E_DB_OK;
    bool	    valid_record;
    GW_RSB	    *rsb = (GW_RSB *)gw_rcb->gwr_gw_rsb;
    GWX_RCB	    gwx_rcb;
    DB_TAB_ID	    tab_id;

    /***FIXME -- this is redundant. ***/
    STRUCT_ASSIGN_MACRO(gw_rcb->gwr_tab_id, tab_id);
    gwx_rcb.xrcb_tab_id = &gw_rcb->gwr_tab_id;
    gw_rcb->gwr_error.err_code = 0;
    gw_rcb->gwr_error.err_data = 0;

    gwx_rcb.xrcb_tab_name = &gw_rcb->gwr_tab_name;
    
    for (;;)
    {

	if ((gw_rcb->gwr_gw_id <= 0) || (gw_rcb->gwr_gw_id >= GW_GW_COUNT) ||
	    (Gwf_facility->gwf_gw_info[gw_rcb->gwr_gw_id].gwf_gw_exist == 0))
	{
	    gwf_error( E_GW0400_BAD_GWID, GWF_INTERR, 1,
			sizeof( gw_rcb->gwr_gw_id ), &gw_rcb->gwr_gw_id );
	    gw_rcb->gwr_error.err_code = E_GW020B_GWR_GET_ERROR;
	    status = E_DB_ERROR;
	    break;
	}

	switch (rsb->gwrsb_access_mode)
	{
	    case    GWT_READ:
	    case    GWT_WRITE:
			break;
	    case    GWT_OPEN_NOACCESS:
	    case    GWT_MODIFY:
	    default:
			gwf_error( E_GW0402_RECORD_ACCESS_CONFLICT, GWF_INTERR,
				0);
			gw_rcb->gwr_error.err_code = E_GW020B_GWR_GET_ERROR;
			status = E_DB_ERROR;
			break;
	}

	if (gw_rcb->gwr_flags & GWR_BYTID)
	{
	    gwx_rcb.xrcb_flags = GWX_BYTID;
	    rsb->gwrsb_tid = gw_rcb->gwr_tid.tid_i4;
	}
	else
	{
	    gwx_rcb.xrcb_flags = GWX_GETNEXT;
	}

	if (status != E_DB_OK)
	    break;
    
	/*
	** Gateway requires two parameters:
	**
	**  GW_RSB describing the record stream from which to 'get'.
	**  buffer in which to place the resulting ingres format row.
	*/
	if (rsb == NULL)
	{
	    (VOID) gwf_error(E_GW0634_NULL_RECORD_STREAM, GWF_INTERR, 0);
	    gw_rcb->gwr_error.err_code = E_GW020B_GWR_GET_ERROR;
	    status = E_DB_ERROR;
	    break;
	}

	gwx_rcb.xrcb_rsb = (PTR)rsb;
	gwx_rcb.xrcb_gw_id = rsb->gwrsb_tcb->gwt_gw_id;
	gwx_rcb.xrcb_gw_session = session;
	STRUCT_ASSIGN_MACRO(gw_rcb->gwr_out_vdata1, gwx_rcb.xrcb_var_data1);
        gwx_rcb.xrcb_access_mode = rsb->gwrsb_access_mode;

        /* schang : pass gateway-specific memory to gateway exit */
        gwx_rcb.xrcb_xhandle =
        Gwf_facility->gwf_gw_info[rsb->gwrsb_tcb->gwt_gw_id].gwf_xhandle;
        gwx_rcb.xrcb_xbitset =
        Gwf_facility->gwf_gw_info[rsb->gwrsb_tcb->gwt_gw_id].gwf_xbitset;

	/*
	** The basic scheme is to loop retrieving records from the gateway
	** until we get a record that matches the user supplied qualification
	** or until we run out of records.  Note that the qualification is
	** optional. If it is not supplied, every record returned from the
	** gateway qualifies.
	*/
	valid_record = FALSE;
	do
	{
	    /* get next record from the gateway */
	    status =
	(*Gwf_facility->gwf_gw_info[rsb->gwrsb_tcb->gwt_gw_id].gwf_gw_exits
							[GWX_VGET])
		(&gwx_rcb);
	    if (status != E_DB_OK && 
                 gwx_rcb.xrcb_error.err_code != E_GW0342_RPTG_OUT_OF_DATA)
	    {
		/*
		** Examine the exit error to see if it is one we 'understand'.
		** Remap the error if appropriate, log it if desired, and then
		** break out of the 'record get' loop to return to our caller.
		*/
		switch ( gwx_rcb.xrcb_error.err_code )
		{
		    case E_GW0641_END_OF_STREAM:
		    case E_GW5411_RMS_RECORD_NOT_FOUND:
			/* End of scan, return this info to caller */
			gw_rcb->gwr_error.err_code = E_GW0641_END_OF_STREAM;
			break;

		    case E_GW5401_RMS_FILE_ACT_ERROR:
			_VOID_ gwf_error(gwx_rcb.xrcb_error.err_code,
					 GWF_INTERR, 0);
			gw_rcb->gwr_error.err_code = E_GW0507_FILE_UNAVAILABLE;
			break;

		    case E_GW540D_RMS_DEADLOCK:
			_VOID_ gwf_error(gwx_rcb.xrcb_error.err_code,
					 GWF_INTERR, 0);
			gw_rcb->gwr_error.err_code = E_GW0508_DEADLOCK;
			break;

		    case E_GW5404_RMS_OUT_OF_MEMORY:
			_VOID_ gwf_error(gwx_rcb.xrcb_error.err_code,
					 GWF_INTERR, 0);
			gw_rcb->gwr_error.err_code = E_GW0600_NO_MEM;
			break;

		    case E_GW540C_RMS_ENQUEUE_LIMIT:
			_VOID_ gwf_error(gwx_rcb.xrcb_error.err_code,
					 GWF_INTERR, 0);
			gw_rcb->gwr_error.err_code =
					 E_GW0504_LOCK_QUOTA_EXCEEDED;
			break;

		    case E_GW5414_RMS_TIMED_OUT:
			_VOID_ gwf_error(gwx_rcb.xrcb_error.err_code,
					 GWF_INTERR, 0);
			gw_rcb->gwr_error.err_code =
					 E_GW0509_LOCK_TIMER_EXPIRED;
			break;

		    case E_GW5484_RMS_DATA_CVT_ERROR:
			gw_rcb->gwr_error.err_code = E_GW050D_DATA_CVT_ERROR;
			break;

		    case E_GW5485_RMS_RECORD_TOO_SHORT:
			if (gwf_errsend(gwx_rcb.xrcb_error.err_code )
				== E_DB_OK)
			{
			    gw_rcb->gwr_error.err_code =
				    E_GW050E_ALREADY_REPORTED;
			}
			else
			{
			    gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR,
				      0);
			    gw_rcb->gwr_error.err_code = E_GW020B_GWR_GET_ERROR;
			}
			break;

		    case E_GW0341_USER_INTR:
			gwx_rcb.xrcb_error.err_code = E_GW050E_ALREADY_REPORTED;
		    case E_GW050E_ALREADY_REPORTED:
		    case E_GW0023_USER_ERROR:
			gw_rcb->gwr_error = gwx_rcb.xrcb_error;
			break;

		    /*
		    ** Most of these RMS errors need to be re-mapped, but I 
		    ** don't yet know what to re-map them to. So, for now,
		    ** we'll just let them fall through into the default
		    ** 'unknown gateway error'
		    */
		    case E_GW540E_RMS_ALREADY_LOCKED:
		    case E_GW540F_RMS_PREVIOUSLY_DELETED:
		    case E_GW5410_RMS_READ_LOCKED_RECORD:
		    case E_GW5412_RMS_READ_AFTER_WAIT:
		    case E_GW5413_RMS_RECORD_LOCKED:
		    case E_GW5415_RMS_RECORD_TOO_BIG:
		    default:
			_VOID_ gwf_error(gwx_rcb.xrcb_error.err_code,
					 GWF_INTERR, 0);
			gw_rcb->gwr_error.err_code = E_GW020B_GWR_GET_ERROR;
			break;
		}
		break;	    /* done reading records */
	    }
            /*
            ** if repeating group of of data, falls through
            */
            if (gwx_rcb.xrcb_error.err_code != E_GW0342_RPTG_OUT_OF_DATA)
            {    
	        gw_rcb->gwr_out_vdata1.data_in_size =
			    gwx_rcb.xrcb_var_data1.data_in_size;
	        gw_rcb->gwr_out_vdata1.data_address =
			    gwx_rcb.xrcb_var_data1.data_address;
	    
	    /*
	    ** Check that the record matches the qualification function if such
	    ** a function has been provided.
	    */
	        if (rsb->gwrsb_qual_func)
	        {
		    session->gws_qfun_adfcb = session->gws_adf_cb;
		    session->gws_qfun_errptr = &gw_rcb->gwr_error;
		    session->gws_adf_cb->adf_errcb.ad_errcode = 0;
		    *rsb->gwrsb_qual_retval = ADE_NOT_TRUE;
		    *rsb->gwrsb_qual_rowaddr =
					gw_rcb->gwr_out_vdata1.data_address;
		    status =
		        (*rsb->gwrsb_qual_func)(session->gws_adf_cb,
                                          rsb->gwrsb_qual_arg);
		    if (status != E_DB_OK)
		    {
			/* If ADF error, issue ADF-specific error msg here.
			** Might even be ignorable...
			*/
			status = gwf_adf_error(&session->gws_adf_cb->adf_errcb,
					status, session, &gw_rcb->gwr_error.err_code);
			if (status != E_DB_OK)
			    break;	/* out of do-while */
		    }
		    valid_record = (*rsb->gwrsb_qual_retval == ADE_TRUE);
	        }
	        else
	        {
		    valid_record = TRUE;
	        }
            }
            /*
            ** schang : support interrupt
            */
            status = gws_check_interrupt(session);
            if (status != E_DB_OK)
            {
        	gw_rcb->gwr_error.err_code = E_GW0341_USER_INTR;
                break;
            }
	} while (valid_record == FALSE);

	/*
	** Note that we can get here for success or failure or 'end of scan'.
	** We don't actually care which -- we just break out of the for loop
	** and return to our caller.
	*/

	break;
    }

    if (status == E_DB_OK)
	gw_rcb->gwr_error.err_code = E_DB_OK;

    /* send tid value (derived from foreign record id) back up to caller */
    gw_rcb->gwr_tid.tid_i4 = rsb->gwrsb_tid;

    /* errptr no longer meaningful, make sure it's cleaned */
    session->gws_qfun_errptr = NULL;

    return(status);
}

/*{
** Name: gwr_put - Add a GW record.
**
** Description:
**	This function performs a record put.  A call to gwt_open() must have
**	preceded this request.  (gwr_position() is not required.)
**
** Inputs:
**	gw_rcb->
**		gw_rsb		table to append
**	  	in_vdata1.
**	    	  data_address	address of the tuple buffer
**	    	  data_in_size	the buffer size
**
** Output:
**	gw_rcb->
**		error.err_code  One of the following error numbers.
**				E_GW020C_GWR_PUT_ERROR
**				E_GW0507_FILE_UNAVAILABLE
**				E_GW050A_RESOURCE_QUOTA_EXCEED
**				E_GW050B_DUPLICATE_KEY
**				E_GW050D_DATA_CVT_ERROR
**				E_GW050E_ALREADY_REPORTED
**				E_GW0600_NO_MEM
**				E_GW050E_ALREADY_REPORTED
**
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
** History:
**      24-Apr-1989 (alexh)
**          Created.
**	28-dec-89 (paul)
**	    Working version created.
**	18-apr-90 (bryanp)
**	    Re-map exit-level errors to generic GWF-level errors as
**	    appropriate. Also upgraded the code to conform to server
**	    conventions (single point of return, etc.)
**	11-jun-1990 (bryanp)
**	    Re-map E_GW5484_RMS_DATA_CVT_ERROR to E_GW050D_DATA_CVT_ERROR.
**	14-jun-1990 (linda, bryanp)
**	    Added support for GWT_OPEN_NOACCESS.
**	18-jun-1990 (bryanp)
**	    Report E_GW541D_RMS_INV_REC_SIZE, then re-map to ALREADY_REPORTED.
**	30-jan-1991 (RickH)
**	    Mapped E_GW5416_RMS_INVALID_DUP_KEY into E_GW050E_ALREADY_REPORTED.
**	8-aug-91 (rickh)
**	    If an error has already been reported to the user, spare us the
**	    alarming yet uninformative error-babble of higher facilities.
**	7-oct-92 (daveb)
**	    pass exit the gw_session, so it can get at a private SCB.
**	    Prototyped.
**      06-nov-92 (schang)
**          assign access_mode  so that exit routines can test the real thing.
*/
DB_STATUS
gwr_put(GW_RCB	*gw_rcb)
{
    GW_SESSION	*session = (GW_SESSION *)gw_rcb->gwr_session_id;
    GW_RSB	*rsb = (GW_RSB *)gw_rcb->gwr_gw_rsb;
    GWX_RCB	gwx_rcb;
    DB_STATUS	status = E_DB_OK;

    for (;;)
    {    
	if ((gw_rcb->gwr_gw_id <= 0) || (gw_rcb->gwr_gw_id >= GW_GW_COUNT) ||
	    (Gwf_facility->gwf_gw_info[gw_rcb->gwr_gw_id].gwf_gw_exist == 0))
	{
	    gwf_error( E_GW0400_BAD_GWID, GWF_INTERR, 1,
			sizeof( gw_rcb->gwr_gw_id ), &gw_rcb->gwr_gw_id );
	    gw_rcb->gwr_error.err_code = E_GW020C_GWR_PUT_ERROR;
	    status = E_DB_ERROR;
	    break;
	}
    
	switch (rsb->gwrsb_access_mode)
	{
	    case    GWT_WRITE:
			break;
	    case    GWT_READ:
	    case    GWT_OPEN_NOACCESS:
	    case    GWT_MODIFY:
	    default:
			gwf_error( E_GW0402_RECORD_ACCESS_CONFLICT, GWF_INTERR,
				0);
			gw_rcb->gwr_error.err_code = E_GW020C_GWR_PUT_ERROR;
			status = E_DB_ERROR;
			break;
	}

	if (status != E_DB_OK)
	    break;
    
	gwx_rcb.xrcb_rsb = (PTR)rsb;
	gwx_rcb.xrcb_gw_session = session;

	STRUCT_ASSIGN_MACRO(gw_rcb->gwr_in_vdata1, gwx_rcb.xrcb_var_data1);
        gwx_rcb.xrcb_access_mode = rsb->gwrsb_access_mode;

	if ((status =
	(*Gwf_facility->gwf_gw_info[rsb->gwrsb_tcb->gwt_gw_id].gwf_gw_exits
							[GWX_VPUT])
	    (&gwx_rcb)) != E_DB_OK)
	{
	    /*
	    ** Examine the exit error to see if it is one we 'understand'.
	    ** Remap the error if appropriate, log it if desired, and then
	    ** break out of the loop to return to our caller.
	    */
	    switch ( gwx_rcb.xrcb_error.err_code )
	    {
		case E_GW5401_RMS_FILE_ACT_ERROR:
		    _VOID_ gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR,
				     0);
		    gw_rcb->gwr_error.err_code = E_GW0507_FILE_UNAVAILABLE;
		    break;

		case E_GW5404_RMS_OUT_OF_MEMORY:
		    _VOID_ gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR,
				     0);
		    gw_rcb->gwr_error.err_code = E_GW0600_NO_MEM;
		    break;

		case E_GW5417_RMS_DEVICE_FULL:
		    _VOID_ gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR,
				     0);
		    gw_rcb->gwr_error.err_code = E_GW050A_RESOURCE_QUOTA_EXCEED;
		    break;

		case E_GW5418_RMS_DUPLICATE_KEY:
		    _VOID_ gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR,
				     0);
		    gw_rcb->gwr_error.err_code = E_GW050B_DUPLICATE_KEY;
		    break;

		case E_GW541D_RMS_INV_REC_SIZE:
		    gw_rcb->gwr_error.err_code = E_GW050E_ALREADY_REPORTED;
		    break;

		case E_GW5484_RMS_DATA_CVT_ERROR:
		    gw_rcb->gwr_error.err_code = E_GW050D_DATA_CVT_ERROR;
		    break;

		case E_GW5416_RMS_INVALID_DUP_KEY:
		    _VOID_ gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR,
				     0);
		    gw_rcb->gwr_error.err_code = E_GW050E_ALREADY_REPORTED;
		    break;

		case E_GW050E_ALREADY_REPORTED:
		    gw_rcb->gwr_error.err_code = E_GW050E_ALREADY_REPORTED;
		    break;

		/*
		** These messages probably need to be mapped to something...
		*/
		case E_GW540E_RMS_ALREADY_LOCKED:
		case E_GW5419_RMS_INDEX_UPDATE_ERROR:
		case E_GW5413_RMS_RECORD_LOCKED:

		default:
		    _VOID_ gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR,
				     0);
		    gw_rcb->gwr_error.err_code = E_GW020C_GWR_PUT_ERROR;
		    break;
	    }
	    break;
	}

	/*
	** Successful end of gwr_put:
	*/
	break;
    }

    return (status);
}

/*{
** Name: gwr_replace - GW record stream replace.
**
** Description:
**	This function performs record replacement.  A call to gwr_position()
**	must have preceded this request.
**
** Inputs:
**	gw_rcb->
**		gw_rsb		record stream
**	  	in_vdata1.
**	    	  data_address	address of the tuple buffer
**	    	  data_in_size	the buffer size
**
** Output:
**	gw_rcb->
**		error.err_code  One of the following error numbers.
**				E_GW020D_GWR_REPLACE_ERROR
**				E_GW0507_FILE_UNAVAILABLE
**				E_GW050B_DUPLICATE_KEY
**				E_GW050C_NOT_POSITIONED
**				E_GW050D_DATA_CVT_ERROR
**				E_GW050E_ALREADY_REPORTED
**				E_GW0600_NO_MEM
**
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
** History:
**      24-Apr-1989 (alexh)
**          Created.
**	18-apr-90 (bryanp)
**	    Re-map exit-level errors to generic GWF-level errors as
**	    appropriate. Also, restructured code to have single point
**	    of return and upgraded to general server conventions.
**	11-jun-1990 (bryanp)
**	    Re-map E_GW5484_RMS_DATA_CVT_ERROR to E_GW050D_DATA_CVT_ERROR.
**	14-jun-1990 (linda, bryanp)
**	    Added support for GWT_OPEN_NOACCESS.
**	30-jan-1991 (RickH)
**	    Mapped E_GW5416_RMS_INVALID_DUP_KEY into E_GW050E_ALREADY_REPORTED.
**	8-aug-91 (rickh)
**	    If an error has already been reported to the user, spare us the
**	    alarming yet uninformative error-babble of higher facilities.
**	7-oct-92 (daveb)
**	    pass gw_session to exit for access to private SCB.  Prototyped.
**      06-nov-92 (schang)
**          assign access_mode  so that exit routines can test the real thing.
*/
DB_STATUS
gwr_replace(GW_RCB	*gw_rcb)
{
    GW_SESSION	*session = (GW_SESSION *)gw_rcb->gwr_session_id;
    GW_RSB	*rsb = (GW_RSB *)gw_rcb->gwr_gw_rsb;
    GWX_RCB	gwx_rcb;
    DB_STATUS	status = E_DB_OK;

    for (;;)
    {
	if ((gw_rcb->gwr_gw_id <= 0) || (gw_rcb->gwr_gw_id >= GW_GW_COUNT) ||
	    (Gwf_facility->gwf_gw_info[gw_rcb->gwr_gw_id].gwf_gw_exist == 0))
	{
	    gwf_error( E_GW0400_BAD_GWID, GWF_INTERR, 1,
			sizeof( gw_rcb->gwr_gw_id ), &gw_rcb->gwr_gw_id );
	    gw_rcb->gwr_error.err_code = E_GW020D_GWR_REPLACE_ERROR;
	    status = E_DB_ERROR;
	    break;
	}
    
	switch (rsb->gwrsb_access_mode)
	{
	    case    GWT_WRITE:
			break;
	    case    GWT_READ:
	    case    GWT_OPEN_NOACCESS:
	    case    GWT_MODIFY:
	    default:
			gwf_error( E_GW0402_RECORD_ACCESS_CONFLICT, GWF_INTERR,
				0);
			gw_rcb->gwr_error.err_code = E_GW020D_GWR_REPLACE_ERROR;
			status = E_DB_ERROR;
			break;
	}

	if (status != E_DB_OK)
	    break;
    
	gwx_rcb.xrcb_rsb = (PTR)rsb;
	gwx_rcb.xrcb_gw_session = session;

	STRUCT_ASSIGN_MACRO(gw_rcb->gwr_in_vdata1, gwx_rcb.xrcb_var_data1);
        gwx_rcb.xrcb_access_mode = rsb->gwrsb_access_mode;

	if ((status = 
	(*Gwf_facility->gwf_gw_info[rsb->gwrsb_tcb->gwt_gw_id].gwf_gw_exits
						    [GWX_VREPLACE])
	    (&gwx_rcb)) != E_DB_OK)
	{
	    /*
	    ** Examine the exit error to see if it is one we 'understand'.
	    ** Remap the error if appropriate, log it if desired, and then
	    ** break out of the loop to return to our caller.
	    */
	    switch ( gwx_rcb.xrcb_error.err_code )
	    {
		case E_GW5401_RMS_FILE_ACT_ERROR:
		    _VOID_ gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR,
				     0);
		    gw_rcb->gwr_error.err_code = E_GW0507_FILE_UNAVAILABLE;
		    break;

		case E_GW541A_RMS_NO_CURRENT_RECORD:
		    _VOID_ gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR,
				     0);
		    gw_rcb->gwr_error.err_code = E_GW050C_NOT_POSITIONED;
		    break;

		case E_GW5404_RMS_OUT_OF_MEMORY:
		    _VOID_ gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR,
				     0);
		    gw_rcb->gwr_error.err_code = E_GW0600_NO_MEM;
		    break;

		case E_GW5418_RMS_DUPLICATE_KEY:
		    _VOID_ gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR,
				     0);
		    gw_rcb->gwr_error.err_code = E_GW050B_DUPLICATE_KEY;
		    break;

		case E_GW541E_RMS_INV_UPD_SIZE:
		    if (gwf_errsend(gwx_rcb.xrcb_error.err_code ) == E_DB_OK)
		    {
			gw_rcb->gwr_error.err_code = E_GW050E_ALREADY_REPORTED;
		    }
		    else
		    {
			gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR, 0);
			gw_rcb->gwr_error.err_code = E_GW020D_GWR_REPLACE_ERROR;
		    }
		    break;

		case E_GW5484_RMS_DATA_CVT_ERROR:
		    gw_rcb->gwr_error.err_code = E_GW050D_DATA_CVT_ERROR;
		    break;

		case E_GW5416_RMS_INVALID_DUP_KEY:
		    _VOID_ gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR,
				     0);
		    gw_rcb->gwr_error.err_code = E_GW050E_ALREADY_REPORTED;
		    break;

		case E_GW050E_ALREADY_REPORTED:
		    gw_rcb->gwr_error.err_code = E_GW050E_ALREADY_REPORTED;
		    break;

		/*
		** These messages probably need to be mapped to something...
		*/
		case E_GW540E_RMS_ALREADY_LOCKED:
		case E_GW5419_RMS_INDEX_UPDATE_ERROR:

		default:
		    _VOID_ gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR,
				     0);
		    gw_rcb->gwr_error.err_code = E_GW020D_GWR_REPLACE_ERROR;
		    break;
	    }
	    break;
	}

	/*
	** Successful end of gwr_replace:
	*/
	break;
    }

    return (status);
}

/*{
** Name: gwr_delete - GW record stream delete.
**
** Description:
**	This function performs deletion of a record from the current record
**	stream position.  A call to gwr_position() or gwr_get() must have
**	preceded this request.
**
** Inputs:
**	gw_rcb->
**		gw_rsb		record stream
**
** Output:
**	gw_rcb->
**		error.err_code  One of the following error numbers.
**				E_GW020E_GWR_DELETE_ERROR
**				E_GW0507_FILE_UNAVAILABLE
**				E_GW050C_NOT_POSITIONED
**				E_GW0600_NO_MEM
**				E_GW050E_ALREADY_REPORTED
**
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
** History:
**      24-Apr-1989 (alexh)
**          Created.
**	18-apr-90 (bryanp)
**	    Re-map exit-level errors to generic GWF-level errors as
**	    appropriate. Also, restructured code to have single point
**	    of return and upgraded to general server conventions.
**	14-jun-1990 (linda, bryanp)
**	    Added support for GWT_OPEN_NOACCESS.
**	8-aug-91 (rickh)
**	    If an error has already been reported to the user, spare us the
**	    alarming yet uninformative error-babble of higher facilities.
**	7-oct-92 (daveb)
**	    Pass gw_session to exit to get to private SCB.  Prototyped.
**      06-nov-92 (schang)
**          assign access_mode  so that exit routines can test the real thing.
*/
DB_STATUS
gwr_delete(GW_RCB	*gw_rcb)
{
    GW_SESSION	*session = (GW_SESSION *)gw_rcb->gwr_session_id;
    GW_RSB	*rsb = (GW_RSB *)gw_rcb->gwr_gw_rsb;
    GWX_RCB	gwx_rcb;
    DB_STATUS	status = E_DB_OK;

    for (;;)
    {
	if ((gw_rcb->gwr_gw_id <= 0) || (gw_rcb->gwr_gw_id >= GW_GW_COUNT) ||
	    (Gwf_facility->gwf_gw_info[gw_rcb->gwr_gw_id].gwf_gw_exist == 0))
	{
	    gwf_error( E_GW0400_BAD_GWID, GWF_INTERR, 1,
			sizeof( gw_rcb->gwr_gw_id ), &gw_rcb->gwr_gw_id );
	    gw_rcb->gwr_error.err_code = E_GW020E_GWR_DELETE_ERROR;
	    status = E_DB_ERROR;
	    break;
	}
    
	switch (rsb->gwrsb_access_mode)
	{
	    case    GWT_WRITE:
			break;
	    case    GWT_READ:
	    case    GWT_OPEN_NOACCESS:
	    case    GWT_MODIFY:
	    default:
			gwf_error( E_GW0402_RECORD_ACCESS_CONFLICT, GWF_INTERR,
				   0 );
			gw_rcb->gwr_error.err_code = E_GW020E_GWR_DELETE_ERROR;
			status = E_DB_ERROR;
			break;
	}

	if (status != E_DB_OK)
	    break;
    
	gwx_rcb.xrcb_rsb = (PTR)rsb;
	gwx_rcb.xrcb_gw_session = session;
        gwx_rcb.xrcb_access_mode = rsb->gwrsb_access_mode;

	if ((status =
	(*Gwf_facility->gwf_gw_info[rsb->gwrsb_tcb->gwt_gw_id].gwf_gw_exits
						    [GWX_VDELETE])
	    (&gwx_rcb)) != E_DB_OK)
	{
	    /*
	    ** Examine the exit error to see if it is one we 'understand'.
	    ** Remap the error if appropriate, log it if desired, and then
	    ** break out of the loop to return to our caller.
	    */
	    switch ( gwx_rcb.xrcb_error.err_code )
	    {
		case E_GW541C_RMS_DELETE_FROM_SEQ:
		    if (gwf_errsend(gwx_rcb.xrcb_error.err_code ) == E_DB_OK)
		    {
			gw_rcb->gwr_error.err_code = E_GW050E_ALREADY_REPORTED;
		    }
		    else
		    {
			gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR, 0);
			gw_rcb->gwr_error.err_code = E_GW020E_GWR_DELETE_ERROR;
		    }
		    break;

		case E_GW5401_RMS_FILE_ACT_ERROR:
		    _VOID_ gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR,
				     0);
		    gw_rcb->gwr_error.err_code = E_GW0507_FILE_UNAVAILABLE;
		    break;

		case E_GW541A_RMS_NO_CURRENT_RECORD:
		    _VOID_ gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR,
				     0);
		    gw_rcb->gwr_error.err_code = E_GW050C_NOT_POSITIONED;
		    break;

		case E_GW5404_RMS_OUT_OF_MEMORY:
		    _VOID_ gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR,
				     0);
		    gw_rcb->gwr_error.err_code = E_GW0600_NO_MEM;
		    break;

		case E_GW050E_ALREADY_REPORTED:
		    gw_rcb->gwr_error.err_code = E_GW050E_ALREADY_REPORTED;
		    break;

		default:
		    _VOID_ gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR,
				     0);
		    gw_rcb->gwr_error.err_code = E_GW020E_GWR_DELETE_ERROR;
		    break;
	    }
	    break;
	}

	/*
	** Successful end of gwr_delete:
	*/
	break;
    }
    return (status);
}
