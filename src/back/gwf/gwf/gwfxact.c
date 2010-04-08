/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <cs.h>
#include    <adf.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <gwf.h>
#include    <gwfint.h>

/**
** Name: GWFXACT.C - generic gateway transaction functions.
**
** Description:
**      This file contains the transaction functions for the GWF Facility.
**
**      This file defines the following external functions:
**    
**	gwx_begin()	-   GW begin transaction.
**	gwx_abort()	-   GW transaction abort.
**	gwx_commit()	-   GW transaction commit
**
** History:
**	04-apr-90 (linda)
**	    Created -- broken out from gwf.c which is now obsolete.
**	17-apr-90 (bryanp)
**	    Call the VBEGIN exit out of gwx_begin.
**	    Updated error handling in all routines to current standard.
**	21-jun-1990 (bryanp)
**	    Correctly coded gwx_begin.
**      18-may-92 (schang)
**          GW merge.
**	    4-feb-91 (linda)
**	        Code cleanup.
**      17-dec-1992 (schang)
**          prototype
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
*/

/*{
** Name: gwx_begin - GW begin transaction.
**
** Description:
**	This function performs begin transaction on GW data.  This request has
**	effect only on DBMS gateways.  (File management type gateways cannot
**	process this request.) However, since DMF has no notion of different
**	types of gateways, it unconditionally calls the exit for this this
**	gateway. That exit may be (and probably is) a NO-OP for most gateway
**	types.
**
**	At the time that we call this gateway, we have no conception of a
**	current database or a current table. It's not really clear which
**	gateway we're supposed to inform. Conceptually, if multiple tables from
**	multiple gateways will be involved in this transaction, ALL of those
**	gateways need to be informed. However, we have no such map of which
**	gateways are involved. Therefore, we notify all existing gateways that
**	this session is beginning a transaction.
**
**	Clearly this scheme is sub-optimal.
**
** Inputs:
**	gw_rcb->
**	  	session_id	current GW session id
**
** Output:
**	gw_rcb->
**		error.err_code  One of the following error numbers.
**				E_GW020F_GWX_BEGIN_ERROR
**
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
** History:
**      25-Apr-1989 (alexh)
**          Created.
**	27-dec-89 (paul)
**	    Add comments on work to do.
**	17-apr-90 (bryanp)
**	    Call the VBEGIN exit out of gwx_begin. Set up the basic GWF
**	    framework so that when we actually encounter a gateway which needs
**	    to do real work here, we are ready to go.
**	21-jun-1990 (bryanp)
**	    Coded this function correctly, as much as possible, now that I
**	    understand what it is supposed to do.
**	10-Nov-1992 (daveb)
**	    Include dmrcb.h for prototypes in gwfint.h.
*/
DB_STATUS
gwx_begin
(
    GW_RCB	*gw_rcb
)
{
    DB_STATUS	    status = E_DB_OK;
    GWX_RCB	    gwx_rcb;
    i4		    gw_id;

    for (;;)
    {
	for (gw_id = 0; status == E_DB_OK && gw_id < GW_GW_COUNT; gw_id++)
	{
	    if ((Gwf_facility->gwf_gw_info[gw_rcb->gwr_gw_id].gwf_gw_exist
		    != 0))
	    {

		/*
		**** What goes into the gwx_rcb for this exit????
		*****AT LEAST THE SESSION ID***
		*/

		status =
		    (*Gwf_facility->gwf_gw_info[gw_id].gwf_gw_exits
						[GWX_VBEGIN])
			(&gwx_rcb);

		if (status != E_DB_OK)
		{
		    gwf_error( gwx_rcb.xrcb_error.err_code, GWF_INTERR, 0 );
		    gw_rcb->gwr_error.err_code = E_GW020F_GWX_BEGIN_ERROR;
		    status = E_DB_ERROR;
		    break;
		}
	    }
	}

	/*
	** Successful end of gwx_begin:
	*/
	status = E_DB_OK;
	break;
    }

    return ( status );
}

/*{
** Name: gwx_abort - GW transaction abort.
**
** Description:
**	This function performs GW transaction abort.   This request has effect
**	only on DBMS gateways.  (File management type gateways cannot process
**	this request.)
**
** Inputs:
**	gw_rcb->
**	  	session_id	current GW session id
**
** Output:
**	gw_rcb->
**		error.err_code  One of the following error numbers.
**				E_GW0210_GWX_ABORT_ERROR
**
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
** History:
**      25-Apr-1989 (alexh)
**          Created.
**	28-dec-89 (paul)
**	    Nodified for new RSB management strategy.
**	17-apr-90 (bryanp)
**	    Added improved error handling. Check return codes, log errors.
*/
DB_STATUS
gwx_abort
(
    GW_RCB	*gw_rcb
)
{
    DB_STATUS	status;
    GW_SESSION	*session = (GW_SESSION *)gw_rcb->gwr_session_id;
    GW_RSB	*rsb;
    GWX_RCB	gwx_rcb;

    /* look for active rsb's of the session */
    status = E_DB_OK;
    for (;;)
    {
	if ((gw_rcb->gwr_gw_id < 0) || (gw_rcb->gwr_gw_id >= GW_GW_COUNT) ||
	    (Gwf_facility->gwf_gw_info[gw_rcb->gwr_gw_id].gwf_gw_exist == 0))
	{
	    gwf_error( E_GW0400_BAD_GWID, GWF_INTERR, 1,
			sizeof(gw_rcb->gwr_gw_id), &gw_rcb->gwr_gw_id);
	    gw_rcb->gwr_error.err_code = E_GW0210_GWX_ABORT_ERROR;
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Loop through each of the currently active record streams and
	** call the exit to abort each stream. It's not clear whether
	** we should break out of the loop if an exit fails or just report
	** the error and keep trying to abort subsequent streams in the
	** list.
	*/

	rsb = session->gws_rsb_list;
	while (rsb)
	{
	    gwx_rcb.xrcb_rsb = (PTR)rsb;
	    status =
		(*Gwf_facility->gwf_gw_info[gw_rcb->gwr_gw_id].gwf_gw_exits
							[GWX_VABORT])
		    (&gwx_rcb);

	    if (status != E_DB_OK)
	    {
		/**** Log errors at this point ? */
		gwf_error( gwx_rcb.xrcb_error.err_code, GWF_INTERR, 0 );
		gw_rcb->gwr_error.err_code = E_GW0210_GWX_ABORT_ERROR;
		status = E_DB_ERROR;
		break;	    /* or perhaps continue?? */
	    }
	    rsb = rsb->gwrsb_next;
	}
	if (status != E_DB_OK)
	{
	    break;
	}

	/*
	** Successful end of gwx_abort:
	*/
	session->gws_txn_begun = FALSE;
	status = E_DB_OK;

	break;
    }

    return ( status );
}

/*{
** Name: gwx_commit - GW transaction commit
**
** Description:
**	This function performs GW transaction commit.  This
**	request has effect only on DBMS gateways.  (File
**	management type gateways cannot process this request.)
**
** Inputs:
**	gw_rcb->
**	  	session_id	current GW session id
**
** Output:
**	gw_rcb->
**		error.err_code  One of the following error numbers.
**				E_GW0211_GWX_COMMIT_ERROR
**				E_GW0507_FILE_UNAVAILABLE
**				E_GW0600_NO_MEM
**
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
** History:
**      25-Apr-1989 (alexh)
**          Created.
**	27-dec-89 (paul)
**	    Add comments on work to do.
**	17-apr-90 (bryanp)
**	    Restructured and added error handling.
*/
DB_STATUS
gwx_commit
(
    GW_RCB	*gw_rcb
)
{
    DB_STATUS	status;
    GW_SESSION	*session = (GW_SESSION *)gw_rcb->gwr_session_id;
    GW_RSB	*rsb;
    GWX_RCB	gwx_rcb;
    
    for (;;)
    {
	if ((gw_rcb->gwr_gw_id < 0) || (gw_rcb->gwr_gw_id >= GW_GW_COUNT) ||
	    (Gwf_facility->gwf_gw_info[gw_rcb->gwr_gw_id].gwf_gw_exist == 0))
	{
	    gwf_error( E_GW0400_BAD_GWID, GWF_INTERR, 1,
			sizeof(gw_rcb->gwr_gw_id), &gw_rcb->gwr_gw_id);
	    gw_rcb->gwr_error.err_code = E_GW0211_GWX_COMMIT_ERROR;
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Loop through each of the currently active record streams and
	** call the exit to commit each stream. It's not clear whether
	** we should break out of the loop if an exit fails or just report
	** the error and keep trying to commit subsequent streams in the
	** list.
	*/

	rsb = session->gws_rsb_list;
	while (rsb)
	{
	    gwx_rcb.xrcb_rsb = (PTR)rsb;
	    status = (*Gwf_facility->gwf_gw_info[gw_rcb->gwr_gw_id].gwf_gw_exits
							[GWX_VCOMMIT])
		    (&gwx_rcb);

	    if (status != E_DB_OK)
	    {
		/**** Log errors at this point ? */
		switch ( gwx_rcb.xrcb_error.err_code )
		{
		    case E_GW5401_RMS_FILE_ACT_ERROR:
			gwf_error( gwx_rcb.xrcb_error.err_code, GWF_INTERR, 0 );
			gw_rcb->gwr_error.err_code = E_GW0507_FILE_UNAVAILABLE;
			break;

		    case E_GW5404_RMS_OUT_OF_MEMORY:
			gwf_error( gwx_rcb.xrcb_error.err_code, GWF_INTERR, 0 );
			gw_rcb->gwr_error.err_code = E_GW0600_NO_MEM;
			break;

		    default:
			gwf_error( gwx_rcb.xrcb_error.err_code, GWF_INTERR, 0 );
			gw_rcb->gwr_error.err_code = E_GW0211_GWX_COMMIT_ERROR;
			break;
		}
		status = E_DB_ERROR;
		break;	    /* or perhaps continue?? */
	    }
	    rsb = rsb->gwrsb_next;
	}
	if (status != E_DB_OK)
	{
	    break;
	}

	/*
	** Successful end of gwx_commit:
	*/
	session->gws_txn_begun = FALSE;
	status = E_DB_OK;

	break;
    }

    return ( status );
}
