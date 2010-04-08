/*
** Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <ci.h>
#include    <er.h>
#include    <gc.h>
#include    <gcccl.h>
#include    <id.h>
#include    <lo.h>
#include    <me.h>
#include    <mu.h>
#include    <pm.h>
#include    <qu.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>

#include    <iicommon.h>
#include    <gca.h>
#include    <gcaint.h>
#include    <gcc.h>
#include    <gcr.h>
#include    <gccal.h>
#include    <gccer.h>
#include    <gccgref.h>

/*
** Name: gcrgcc.c
**
** Description:
**	Embedded Comm Server interface.
**
** History:
**	23-Oct-95 (gordy)
**	    Created.
**	 9-Jan-96 (gordy)
**	    Cleaned up a bit of the tracing.
**	18-Jan-96 (gordy)
**	    Set GCC parmlist when initiating callback to Comm Server
**	    which did not originate from the Comm Server (GCA_REQUEST).
**	22-Feb-96 (gordy)
**	    Check for time-out requests on GCA receive requests.  Polling
**	    time-outs (time-out = 0) should return with a time-out error
**	    if no message available.  We cannot currently support other
**	    time-out values (time-out > 0), so we return a parameter 
**	    error.  No change to blocking requests (time-out < 0).
**	23-Feb-96 (gordy)
**	    Added a missing break statement to the time-out support code
**	    in the preceeding fix.  The first would produce the time-out
**	    error, but would also note the receive request such that
**	    subsequent requests would produce duplicate receive errors.
**	27-Feb-96 (rajus01)
**	    Added CI_INGRES_NET parameter to gcc_init(). This enables
**	    Protocol Bridge check for  it's own AUTH STRING.
**	27-May-97 (thoda04)
**	    Included id.h, st.h, and tr.h headers for the function prototypes.
**	10-oct-1996 (canor01)
**	    Use SystemCfgPrefix to allow system variable override.
**	 4-Jul-1999 (rajus01)
**	    Removed references to acb, GCsendpeer(), GCrecvpeer(),
**	    security labels. Security labels are sent as 
**	    part of GCA_AUX_DATA.
**	    
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

#ifdef GCF_EMBEDDED_GCC

/*
** Forward references.
*/
static i4	gcr_eval( GCR_CB *, i4, bool );
static STATUS	gcr_init_gcc( VOID );
static STATUS	gcr_peer_info( GCR_CB * );
static STATUS	gcr_fs_info( GCR_CB * );
static STATUS	gcr_request( GCR_CB * );
static STATUS	gcr_response( GCR_CB * );
static STATUS	gcr_send( GCR_CB *, i4, bool *, bool * );
static STATUS	gcr_receive( GCR_CB *, i4, bool *, bool * );
static VOID	gcr_abort( GCR_CB *, i4  );


/*
** Name: gcr_states
**
** Description:
**	Defines next state and action for each current state
**	and input combination in the embedded Comm Server
**	interface State Machine.
**
** History:
**	23-Oct-95 (gordy)
**	    Created.
*/

static struct
{

    i4		state;
    i4		action;

} gcr_states[ GCR_S_COUNT ][ GCR_I_COUNT ] =
{   
    /* GCR_S_INIT */
    {
	{ GCR_S_INIT,	GCR_A_UNKNOWN },	/* GCR_I_UNKNOWN */
	{ GCR_S_RQST,	GCR_A_RQST },		/* GCR_I_RQST */
	{ GCR_S_INIT,	GCR_A_INVALID },	/* GCR_I_SPEER */
	{ GCR_S_INIT,	GCR_A_INVALID },	/* GCR_I_FS */
	{ GCR_S_INIT,	GCR_A_INVALID },	/* GCR_I_RPEER */
	{ GCR_S_INIT,	GCR_A_INVALID },	/* GCR_I_RQRESP */
	{ GCR_S_INIT,	GCR_A_INVALID },	/* GCR_I_RECEIVE */
	{ GCR_S_INIT,	GCR_A_INVALID },	/* GCR_I_SEND */
	{ GCR_S_INIT,	GCR_A_INVALID },	/* GCR_I_DATA */
	{ GCR_S_INIT,	GCR_A_INVALID },	/* GCR_I_NEED */
	{ GCR_S_INIT,	GCR_A_INVALID },	/* GCR_I_PURGE */
	{ GCR_S_INIT,	GCR_A_INVALID },	/* GCR_I_DISCONN */
	{ GCR_S_INIT,	GCR_A_INVALID },	/* GCR_I_ABORT */
	{ GCR_S_INIT,	GCR_A_INVALID },	/* GCR_I_INVALID */
    },
    /* GCR_S_RQST */
    {
	{ GCR_S_RQST,	GCR_A_UNKNOWN },	/* GCR_I_UNKNOWN */
	{ GCR_S_RQST,	GCR_A_INVALID },	/* GCR_I_RQST */
	{ GCR_S_SPEER,	GCR_A_SPEER },		/* GCR_I_SPEER */
	{ GCR_S_RQST,	GCR_A_INVALID },	/* GCR_I_FS */
	{ GCR_S_RQST,	GCR_A_INVALID },	/* GCR_I_RPEER */
	{ GCR_S_RQST,	GCR_A_INVALID },	/* GCR_I_RQRESP */
	{ GCR_S_RQST,	GCR_A_INVALID },	/* GCR_I_RECEIVE */
	{ GCR_S_RQST,	GCR_A_INVALID },	/* GCR_I_SEND */
	{ GCR_S_RQST,	GCR_A_INVALID },	/* GCR_I_DATA */
	{ GCR_S_RQST,	GCR_A_INVALID },	/* GCR_I_NEED */
	{ GCR_S_RQST,	GCR_A_INVALID },	/* GCR_I_PURGE */
	{ GCR_S_DONE,	GCR_A_NOOP },		/* GCR_I_DISCONN */
	{ GCR_S_RQST,	GCR_A_INVALID },	/* GCR_I_ABORT */
	{ GCR_S_RQST,	GCR_A_INVALID },	/* GCR_I_INVALID */
    },
    /* GCR_S_SPEER */
    {
	{ GCR_S_SPEER,	GCR_A_UNKNOWN },	/* GCR_I_UNKNOWN */
	{ GCR_S_SPEER,	GCR_A_INVALID },	/* GCR_I_RQST */
	{ GCR_S_SPEER,	GCR_A_INVALID },	/* GCR_I_SPEER */
	{ GCR_S_FS,	GCR_A_FS },		/* GCR_I_FS */
	{ GCR_S_RPEER,	GCR_A_RPEER },		/* GCR_I_RPEER */
	{ GCR_S_SPEER,	GCR_A_INVALID },	/* GCR_I_RQRESP */
	{ GCR_S_SPEER,	GCR_A_INVALID },	/* GCR_I_RECEIVE */
	{ GCR_S_SPEER,	GCR_A_INVALID },	/* GCR_I_SEND */
	{ GCR_S_SPEER,	GCR_A_INVALID },	/* GCR_I_DATA */
	{ GCR_S_SPEER,	GCR_A_INVALID },	/* GCR_I_NEED */
	{ GCR_S_SPEER,	GCR_A_INVALID },	/* GCR_I_PURGE */
	{ GCR_S_DONE,	GCR_A_NOOP },		/* GCR_I_DISCONN */
	{ GCR_S_SPEER,	GCR_A_INVALID },	/* GCR_I_ABORT */
	{ GCR_S_SPEER,	GCR_A_INVALID },	/* GCR_I_INVALID */
    },
    /* GCR_S_FS */
    {
	{ GCR_S_FS,	GCR_A_UNKNOWN },	/* GCR_I_UNKNOWN */
	{ GCR_S_FS,	GCR_A_INVALID },	/* GCR_I_RQST */
	{ GCR_S_FS,	GCR_A_INVALID },	/* GCR_I_SPEER */
	{ GCR_S_FS,	GCR_A_INVALID },	/* GCR_I_FS */
	{ GCR_S_RPEER,	GCR_A_RPEER },		/* GCR_I_RPEER */
	{ GCR_S_FS,	GCR_A_INVALID },	/* GCR_I_RQRESP */
	{ GCR_S_FS,	GCR_A_INVALID },	/* GCR_I_RECEIVE */
	{ GCR_S_FS,	GCR_A_INVALID },	/* GCR_I_SEND */
	{ GCR_S_FS,	GCR_A_INVALID },	/* GCR_I_DATA */
	{ GCR_S_FS,	GCR_A_INVALID },	/* GCR_I_NEED */
	{ GCR_S_FS,	GCR_A_INVALID },	/* GCR_I_PURGE */
	{ GCR_S_DONE,	GCR_A_NOOP },		/* GCR_I_DISCONN */
	{ GCR_S_FS,	GCR_A_INVALID },	/* GCR_I_ABORT */
	{ GCR_S_FS,	GCR_A_INVALID },	/* GCR_I_INVALID */
    },
    /* GCR_S_RPEER */
    {
	{ GCR_S_RPEER,	GCR_A_UNKNOWN },	/* GCR_I_UNKNOWN */
	{ GCR_S_RPEER,	GCR_A_INVALID },	/* GCR_I_RQST */
	{ GCR_S_RPEER,	GCR_A_INVALID },	/* GCR_I_SPEER */
	{ GCR_S_RPEER,	GCR_A_INVALID },	/* GCR_I_FS */
	{ GCR_S_RPEER,	GCR_A_INVALID },	/* GCR_I_RPEER */
	{ GCR_S_CONN,	GCR_A_RQRESP },		/* GCR_I_RQRESP */
	{ GCR_S_RPEER,	GCR_A_INVALID },	/* GCR_I_RECEIVE */
	{ GCR_S_RPEER,	GCR_A_INVALID },	/* GCR_I_SEND */
	{ GCR_S_RPEER,	GCR_A_INVALID },	/* GCR_I_DATA */
	{ GCR_S_RPEER,	GCR_A_INVALID },	/* GCR_I_NEED */
	{ GCR_S_RPEER,	GCR_A_INVALID },	/* GCR_I_PURGE */
	{ GCR_S_RPEER,	GCR_A_INVALID },	/* GCR_I_DISCONN */
	{ GCR_S_RPEER,	GCR_A_INVALID },	/* GCR_I_ABORT */
	{ GCR_S_RPEER,	GCR_A_INVALID },	/* GCR_I_INVALID */
    },
    /* GCR_S_CONN */
    {
	{ GCR_S_CONN,	GCR_A_UNKNOWN },	/* GCR_I_UNKNOWN */
	{ GCR_S_CONN,	GCR_A_INVALID },	/* GCR_I_RQST */
	{ GCR_S_CONN,	GCR_A_INVALID },	/* GCR_I_SPEER */
	{ GCR_S_CONN,	GCR_A_INVALID },	/* GCR_I_FS */
	{ GCR_S_CONN,	GCR_A_INVALID },	/* GCR_I_RPEER */
	{ GCR_S_CONN,	GCR_A_INVALID },	/* GCR_I_RQRESP */
	{ GCR_S_CONN,	GCR_A_RECEIVE },	/* GCR_I_RECEIVE */
	{ GCR_S_CONN,	GCR_A_SEND },		/* GCR_I_SEND */
	{ GCR_S_CONN,	GCR_A_DATA },		/* GCR_I_DATA */
	{ GCR_S_CONN,	GCR_A_NEED },		/* GCR_I_NEED */
	{ GCR_S_CONN,	GCR_A_PURGE },		/* GCR_I_PURGE */
	{ GCR_S_DISCON,	GCR_A_DISCONN },	/* GCR_I_DISCONN */
	{ GCR_S_DISCON,	GCR_A_DISCONN },	/* GCR_I_ABORT */
	{ GCR_S_CONN,	GCR_A_INVALID },	/* GCR_I_INVALID */
    },
    /* GCR_S_DISCONN */
    {
	{ GCR_S_DISCON,	GCR_A_UNKNOWN },	/* GCR_I_UNKNOWN */
	{ GCR_S_DISCON,	GCR_A_INVALID },	/* GCR_I_RQST */
	{ GCR_S_DISCON,	GCR_A_INVALID },	/* GCR_I_SPEER */
	{ GCR_S_DISCON,	GCR_A_INVALID },	/* GCR_I_FS */
	{ GCR_S_DISCON,	GCR_A_INVALID },	/* GCR_I_RPEER */
	{ GCR_S_DISCON,	GCR_A_INVALID },	/* GCR_I_RQRESP */
	{ GCR_S_DISCON,	GCR_A_ERROR },		/* GCR_I_RECEIVE */
	{ GCR_S_DISCON,	GCR_A_ERROR },		/* GCR_I_SEND */
	{ GCR_S_DISCON,	GCR_A_ERROR },		/* GCR_I_DATA */
	{ GCR_S_DISCON,	GCR_A_ERROR },		/* GCR_I_NEED */
	{ GCR_S_DISCON,	GCR_A_INVALID },	/* GCR_I_PURGE */
	{ GCR_S_DONE,	GCR_A_DONE },		/* GCR_I_DISCONN */
	{ GCR_S_DONE,	GCR_A_DONE },		/* GCR_I_ABORT */
	{ GCR_S_DISCON,	GCR_A_INVALID },	/* GCR_I_INVALID */
    },
    /* GCR_S_DONE */
    {
	{ GCR_S_DONE,	GCR_A_UNKNOWN },	/* GCR_I_UNKNOWN */
	{ GCR_S_DONE,	GCR_A_INVALID },	/* GCR_I_RQST */
	{ GCR_S_DONE,	GCR_A_INVALID },	/* GCR_I_SPEER */
	{ GCR_S_DONE,	GCR_A_INVALID },	/* GCR_I_FS */
	{ GCR_S_DONE,	GCR_A_INVALID },	/* GCR_I_RPEER */
	{ GCR_S_DONE,	GCR_A_INVALID },	/* GCR_I_RQRESP */
	{ GCR_S_DONE,	GCR_A_INVALID },	/* GCR_I_RECEIVE */
	{ GCR_S_DONE,	GCR_A_INVALID },	/* GCR_I_SEND */
	{ GCR_S_DONE,	GCR_A_INVALID },	/* GCR_I_DATA */
	{ GCR_S_DONE,	GCR_A_INVALID },	/* GCR_I_NEED */
	{ GCR_S_DONE,	GCR_A_INVALID },	/* GCR_I_PURGE */
	{ GCR_S_DONE,	GCR_A_INVALID },	/* GCR_I_DISCONN */
	{ GCR_S_DONE,	GCR_A_INVALID },	/* GCR_I_ABORT */
	{ GCR_S_DONE,	GCR_A_INVALID },	/* GCR_I_INVALID */
    },
};


/*
** Name gcr_gcc_sm
**
** Description:
**	Embedded Comm Server interface State Machine.
**
** Input:
**	rcb		Router Control Block.
**	request		Router request ID.
**	down		TRUE if request made by GCA, FALSE if GCC.
**
** Output:
**	None.
**
** Returns:
**	STATUS
**
** History:
**	23-Oct-95 (gordy)
**	    Created.
**	22-Feb-96 (gordy)
**	    Check for time-out requests on GCA receive requests.  Polling
**	    time-outs (time-out = 0) should return with a time-out error
**	    if no message available.  We cannot currently support other
**	    time-out values (time-out > 0), so we return a parameter
**	    error.  No change to blocking requests (time-out < 0).
**	23-Feb-96 (gordy)
**	    Added a missing break statement to the time-out support code
**	    in the preceeding fix.  The first would produce the time-out
**	    error, but would also note the receive request such that
**	    subsequent requests would produce duplicate receive errors.
*/

STATUS
gcr_gcc_sm( GCR_CB *rcb, i4  request, bool down )
{
    GCR_SR_INFO	*recv, *send;
    i4		input, subchan;
    i4		current_state = rcb->state;
    bool	gca_callback = FALSE;
    bool	gcc_callback = FALSE;
    STATUS	status = OK;

    /* 
    ** Validate current state 
    */
    if ( rcb->state < 0  ||  rcb->state >= GCR_S_COUNT )
    {
	GCA_DEBUG_MACRO(5)( "gcr_gcc_sm: invalid RCB state %d\n", rcb->state );
	status = E_GC0003_INV_SVC_CODE;

	if ( down )  
	    gca_callback = TRUE;
	else
	    gcc_callback = TRUE;

	goto complete;
    }

    /*
    ** Determine input event from current state and request.
    */
    input = gcr_eval( rcb, request, down );

    /*
    ** Get next state and action from current state and input event.
    */

    rcb->state = gcr_states[ current_state ][ input ].state;


    GCA_DEBUG_MACRO(4)( "%04d     GCR %d %d -> %d %d\n",
	rcb->assoc_id, current_state, input, rcb->state,
	gcr_states[ current_state ][ input ].action );

    /*
    ** Process the request.  Note that most actions will only
    ** occur for a known request direction (down TRUE or FALSE),
    ** as controlled by gcr_eval() and the state tables.
    */

    switch( gcr_states[ current_state ][ input ].action )
    {
	case GCR_A_NOOP :
	    /*
	    ** Nothing to do, return OK.
	    */
	    if ( down )
		gca_callback = TRUE;
	    else
		gcc_callback = TRUE;
	    break;

	case GCR_A_RQST :
	    /*
	    ** The embedded Comm Server is not initialized
	    ** until the first remote request.  If the GCC
	    ** global data area has not been allocated, 
	    ** then the embedded Comm Server needs to be 
	    ** initialized.
	    */
	    if ( ! IIGCc_global )  status = gcr_init_gcc();

	    /*
	    ** This is a request to connect to the Comm Server.
	    ** For all practical purposes this was accomplished
	    ** by initializing the embedded Comm Server.  The
	    ** GCrequest() service parms only has the Comm Server 
	    ** address, which is of no interest to us.  The remote
	    ** connection info will be passed to us later in the 
	    ** peer info.  We be done!  Callback GCA to continue
	    ** processing.
	    */
	    rcb->svc_parms->size_advise = GCA_NORM_SIZE;
	    gca_callback = TRUE;
	    break;
	
	case GCR_A_SPEER :
	    /*
	    ** Since the peer info may be followed by optional
	    ** info (security label and/or fast select info), 
	    ** save the peer info until we are ready to complete 
	    ** the connection request.  Callback GCA to continue 
	    ** processing.
	    */
	    status = gcr_peer_info( rcb );
	    gca_callback = TRUE;
	    break;

	case GCR_A_FS :
	    /*
	    ** While no additional info will be sent by GCA for the
	    ** connection request, fast select info is optional so
	    ** we must save it until we reach a non-optional point
	    ** at which we can complete the connection request. 
	    ** Callback GCA to continue processing.
	    */
	    status = gcr_fs_info( rcb );
	    gca_callback = TRUE;
	    break;

	case GCR_A_RPEER :
	    /*
	    ** The client GCA issues a peer receive once all
	    ** request info has been sent to the server.  We
	    ** now invoke the embedded Comm Server as a call-
	    ** back as if a GCA_LISTEN has completed so that 
	    ** the connect request may be processed.  Note 
	    ** that we do not make a GCA callback at this time.  
	    ** At some point, GCC will issue a GCA_RQRESP call 
	    ** which will be our signal to return the peer 
	    ** information to GCA.
	    */
	    status = gcr_request( rcb );	/* Build GCA_LISTEN parms */
	    gcc_callback = TRUE;
	    break;

	case GCR_A_RQRESP :
	    /*
	    ** The embedded Comm Server has finished processing
	    ** a connection request.  We now need to simulate
	    ** the completion of the receive peer request by
	    ** building the new peer info and making the GCA
	    ** callback.  We also need to callback the embedded
	    ** Comm Server to complete its GCA_RQRESP request.
	    ** While this is a GCC request, the GCA service
	    ** parameter list was saved when the GCA receive
	    ** peer request was made.
	    */
	    status = gcr_response( rcb );
	    gcc_callback = TRUE;
	    gca_callback = TRUE;
	    break;

	case GCR_A_RECEIVE :
	    /*
	    ** GCA is ready for a message.  If the embedded Comm
	    ** Server has sent us data (GCR_A_DATA), we complete
	    ** both the GCA and GCC request.  Otherwise, we hold 
	    ** the GCA request until GCC provides the data.
	    */
	    subchan = rcb->svc_parms->flags.flow_indicator ? GCA_EXPEDITED 
							   : GCA_NORMAL;
	    recv = &rcb->recv[ subchan ];

	    if ( recv->flags & GCR_F_GCA )
	    {
		/*
		** This should not happen since multiple
		** concurrent GCA_RECEIVEs on the same
		** channel are not allowed, but make
		** sure anyway.
		*/
		status = E_GC0024_DUP_REQUEST;
		gca_callback = TRUE;
		break;
	    }
	    else if (recv->flags & GCR_F_PURGE && ! (recv->flags & GCR_F_CONT))
	    {
		/*
		** Purge the GCA receive request by
		** returning the new_chop indicator
		** and returning a 0 length message.
		*/
		GCA_DEBUG_MACRO(4)( "%04d     GCR purging GCA request.\n", 
				    rcb->assoc_id );
		recv->flags &= ~GCR_F_PURGE;
		rcb->svc_parms->flags.new_chop = 1;
		rcb->svc_parms->rcv_data_length = 0;
		gca_callback = TRUE;
		break;
	    }
	    else  if ( rcb->svc_parms->time_out >= 0  &&  
		       ! (recv->flags & GCR_F_GCC) )
	    {
		/*
		** GCA has requested a time-out and there is no
		** GCC message available to satisfy the current
		** request.  We currently don't have a way to
		** handle a real time-out value (time-out if no
		** message received after some non-zero time
		** period) but we can handle a polling time-out
		** (if time-out value is 0, complete immediatly).
		*/
		if ( rcb->svc_parms->time_out > 0 )
		    status = E_GC0002_INV_PARM;
		else
		    status = GC_TIME_OUT;

		gca_callback = TRUE;
		break;
	    }

	    /*
	    ** Signal GCA waiting for data.
	    */
	    recv->o_buf_ptr = rcb->svc_parms->svc_buffer;
	    recv->o_buf_len = rcb->svc_parms->reqd_amount;
	    recv->o_msg_len = 0;
	    recv->svc_parms = rcb->svc_parms;
	    recv->flags |= GCR_F_GCA;

	    /*
	    ** Check to see if GCC has provided
	    ** the data GCA is waiting for.
	    */
	    if ( recv->flags & GCR_F_GCC )
		status = gcr_receive( rcb, subchan,
				      &gca_callback, &gcc_callback );
	    break;
	
	case GCR_A_DATA :
	    /*
	    ** The embedded Comm Server has received a message.
	    ** If GCA is waiting on a receive (GCR_A_RECEIVE), 
	    ** we complete both the GCA and GCC request.  Other-
	    ** wise, we hold the GCC request until GCA is ready 
	    ** for data.
	    */
	    {
		GCA_SD_PARMS *sd_parms = (GCA_SD_PARMS *)rcb->parmlist;

		/*
		** GCA_SEND does not specify the channel, 
		** it is determined by the message type.
		*/
		subchan = ( sd_parms->gca_message_type == GCA_ATTENTION  ||  
			    sd_parms->gca_message_type == GCA_NP_INTERRUPT )
			  ? GCA_EXPEDITED : GCA_NORMAL;

		recv = &rcb->recv[ subchan ];

		if ( recv->flags & GCR_F_GCC )
		{
		    /*
		    ** This should not happen since multiple
		    ** concurrent GCA_SENDs on the same
		    ** channel are not allowed, but make
		    ** sure anyway.
		    */
		    status = E_GC0024_DUP_REQUEST;
		    gcc_callback = TRUE;
		    break;
		}
		else  if ( recv->flags & GCR_F_IACK )
		{
		    /*
		    ** During purging, reject all GCC messages
		    ** until a GCA_IACK message is sent.
		    */
		    if ( sd_parms->gca_message_type == GCA_IACK )
			recv->flags &= ~GCR_F_IACK;
		    else
		    {
			status = E_GC0022_NOT_IACK;
			gcc_callback = TRUE;
			break;
		    }
		}

		/*
		** Signal GCC data available.
		**
		** The initial send buffer points to an area
		** for GCA to build the message header, followed
		** by the actual data.  We build the message
		** header directly in the output buffer, so
		** skip this portion of the input buffer.
		*/
		recv->i_buf_ptr = sd_parms->gca_buffer + 
				  IIGCc_global->gcc_gca_hdr_lng;
		recv->i_buf_len = sd_parms->gca_msg_length;
		recv->parmlist = rcb->parmlist;
		recv->mde = rcb->mde;
		recv->flags |= GCR_F_GCC;

		/*
		** Check to see if GCA is waiting
		** for the data GCC is providing.
		*/
		if ( recv->flags & GCR_F_GCA )
		    status = gcr_receive( rcb, subchan,
					  &gca_callback, &gcc_callback );
	    }
	    break;

	case GCR_A_SEND :
	    /*
	    ** GCA wants to send a message.  If the embedded Comm 
	    ** Server is waiting on a receive (GCR_A_NEED), we
	    ** complete both the GCA and GCC request.  Otherwise, 
	    ** we hold the GCA request until GCC is ready for data.
	    */
	    subchan = rcb->svc_parms->flags.flow_indicator;
	    send = &rcb->send[ subchan ];

	    if ( send->flags & GCR_F_GCA )
	    {
		/*
		** This should not happen since multiple
		** concurrent GCA_SENDs on the same
		** channel are not allowed, but make
		** sure anyway.
		*/
		status = E_GC0024_DUP_REQUEST;
		gca_callback = TRUE;
		break;
	    }

	    /*
	    ** Signal GCA data available.
	    */
	    send->i_buf_ptr = rcb->svc_parms->svc_buffer;
	    send->i_buf_len = rcb->svc_parms->svc_send_length;
	    send->svc_parms = rcb->svc_parms;
	    send->flags |= GCR_F_GCA;

	    /*
	    ** Check to see if GCC is waiting
	    ** for the data GCA is providing.
	    */
	    if ( send->flags & GCR_F_GCC )
		status = gcr_send( rcb, subchan, 
				   &gca_callback, &gcc_callback );
	    break;

	case GCR_A_NEED :
	    /*
	    ** The embedded Comm Server is ready for a message.
	    ** If GCA has sent us data (GCR_A_SEND), we complete
	    ** both the GCA and GCC request.  Otherwise, we hold 
	    ** the GCC request until GCA provides the data.
	    */
	    {
		GCA_RV_PARMS *rv_parms = (GCA_RV_PARMS *)rcb->parmlist;

		subchan = rv_parms->gca_flow_type_indicator;
		send = &rcb->send[ subchan ];

		if ( send->flags & GCR_F_GCC )
		{
		    /*
		    ** This should not happen since multiple
		    ** concurrent GCA_RECEIVEs on the same
		    ** channel are not allowed, but make
		    ** sure anyway.
		    */
		    status = E_GC0024_DUP_REQUEST;
		    gcc_callback = TRUE;
		    break;
		}
		else  if ( send->flags & GCR_F_PURGE )
		{
		    /*
		    ** Complete the GCC receive request 
		    ** with a purge error.  Turn off the
		    ** purge indicator.
		    */
		    send->flags &= ~GCR_F_PURGE;
		    status = E_GC0027_RQST_PURGED;
		    gcc_callback = TRUE;
		    break;
		}

		/*
		** Signal GCC waiting for data.
		**
		** Leave room for the header at 
		** the start of the buffer.
		*/
		send->o_buf_ptr = rv_parms->gca_buffer + 
				  IIGCc_global->gcc_gca_hdr_lng;
		send->o_buf_len = rv_parms->gca_b_length - 
				  IIGCc_global->gcc_gca_hdr_lng;
		send->o_msg_len = 0;
		send->parmlist = rcb->parmlist;
		send->mde = rcb->mde;
		send->flags |= GCR_F_GCC;

		/*
		** Check to see if GCA has provided 
		** the data GCC is waiting for.
		*/
		if ( send->flags & GCR_F_GCA )
		    status = gcr_send( rcb, subchan,
				       &gca_callback, &gcc_callback );
	    }
	    break;

	case GCR_A_PURGE :
	    /*
	    ** GCA request to purge normal channel.  GCA
	    ** does this after a GCA_ATTENTION message has
	    ** been sent.  That means we just received this 
	    ** message on the GCC side, effectively putting 
	    ** us in the rpurging state (see RV_DONE in GCA).
	    ** Purging should abort the current (or next) 
	    ** normal channel GCC receive (see RV_RECVCMP 
	    ** and RV_PURGED).  GCA does not expect a callback 
	    ** for this request.
	    */
	    send = &rcb->send[ GCA_NORMAL ];

	    if ( send->flags & GCR_F_CONT  ||  send->hdr_len )
	    {
		/*
		** There is an incomplete send in progress on
		** the channel.  We do not expect to see this
		** condition and are not sure what to do if it
		** occurs.  Trace the problem for now.
		*/
		GCA_DEBUG_MACRO(5)("gcr_gcc_sm: purging incomplete message?\n");
	    }

	    /*
	    ** If the embedded Comm Server is not currently
	    ** waiting for data, flag the need to purge the
	    ** next GCC receive request.
	    */
	    if ( ! (send->flags & GCR_F_GCC) )
		send->flags |= GCR_F_PURGE;
	    else
	    {
		/*
		** Complete the GCC receive request 
		** with a purge error.
		*/
		send->flags &= ~GCR_F_GCC;
		rcb->mde = send->mde;
		rcb->parmlist = send->parmlist;
		((GCA_ALL_PARMS *)rcb->parmlist)->gca_status = 
							E_GC0027_RQST_PURGED;
		gcc_callback = TRUE;
	    }
	    break;

	case GCR_A_DISCONN :
	    /*
	    ** Both GCA and the embedded Comm Server must
	    ** request a disconnect, this being the first
	    ** request.  Abort any outstanding send/receive
	    ** requests from the disconnect requestor.  If 
	    ** this is not an orderly shutdown (we have not 
	    ** processed a GCA_RELEASE message), shutdown 
	    ** the connection by aborting all requests.  We
	    ** don't make the callback until the second
	    ** disconnect request is received (see GCR_A_DONE).
	    */
	    if ( down )
	    {
		rcb->conn.svc_parms = rcb->svc_parms;
		gcr_abort( rcb, (rcb->flags & GCR_F_RELEASE)
				? GCR_F_GCA : (GCR_F_GCA | GCR_F_GCC) );
	    }
	    else
	    {
		rcb->conn.mde = rcb->mde;
		rcb->conn.parmlist = rcb->parmlist;
		gcr_abort( rcb, (rcb->flags & GCR_F_RELEASE)
				? GCR_F_GCC : (GCR_F_GCC | GCR_F_GCA) );
	    }
	    break;
	
	case GCR_A_DONE :
	    /*
	    ** Both GCA and the embedded Comm Server
	    ** have requested disconnects.  Abort any 
	    ** outstanding send/receive requests from 
	    ** the disconnect requestor.  Callback both
	    ** GCA and the embedded Comm Server, being
	    ** sure to set the return info for both
	    ** callbacks.
	    */
	    if ( down )
	    {
		gcr_abort( rcb, GCR_F_GCA );
		rcb->mde = rcb->conn.mde;
		rcb->parmlist = rcb->conn.parmlist;
		((GCA_ALL_PARMS *)rcb->parmlist)->gca_status = OK;
	    }
	    else
	    {
		gcr_abort( rcb, GCR_F_GCC );
		rcb->svc_parms = rcb->conn.svc_parms;
		rcb->svc_parms->status = OK;
	    }

	    gca_callback = TRUE;
	    gcc_callback = TRUE;
	    break;

	case GCR_A_ERROR :
	    /*
	    ** Disconnect in progress,
	    ** fail the current request.
	    */
	    status = E_GC0023_ASSOC_RLSED;

	    if ( down )
		gca_callback = TRUE;
	    else
		gcc_callback = TRUE;
	    break;

	case GCR_A_INVALID :
	    /* Invalid Input */
	    status = E_GC0003_INV_SVC_CODE;
	    GCA_DEBUG_MACRO(5)
		    ( "gcr_gcc_sm: request invalid in current state\n" );

	    if ( down )  
		gca_callback = TRUE;
	    else
		gcc_callback = TRUE;
	    break;
	
	case GCR_A_UNKNOWN :
	    /* Unknown Input */
	    status = E_GC0003_INV_SVC_CODE;
	    GCA_DEBUG_MACRO(5)( "gcr_gcc_sm: unsupported %s request code %d\n",
				down ? "gca" : "gcc", request );

	    if ( down )  
		gca_callback = TRUE;
	    else
		gcc_callback = TRUE;
	    break;
	
	default :
	    /* Unknown action - problem with gcr_states */
	    status = E_GC0003_INV_SVC_CODE;
	    GCA_DEBUG_MACRO(5)( "gcr_gcc_sm: unknown action code %d\n",
				gcr_states[ current_state ][ input ].action );

	    if ( down )  
		gca_callback = TRUE;
	    else
		gcc_callback = TRUE;
	    break;
    }

  complete:

    /*
    ** Return the status to the caller.  If a callback is
    ** requested which does not match the request direction,
    ** the svc_parms/parmlist status value must have already 
    ** been set.
    */
    if ( down )  
	rcb->svc_parms->status = status;
    else
	((GCA_ALL_PARMS *)rcb->parmlist)->gca_status = status;

    /*
    ** We call back the embedded Comm Server before GCA because
    ** GCC usually will do very little work for our callback.
    ** GCA on the other hand will very likely either callback
    ** the client or make another call into the router.
    */
    if ( gcc_callback )  
    {
	GCA_DEBUG_MACRO(4)( "%04d     GCR making GCC callback: status = 0x%x\n",
			    rcb->assoc_id,
			    ((GCA_ALL_PARMS *)rcb->parmlist)->gca_status );

	gcc_gca_exit( rcb->mde );
	if ( ! down )  status = OK;	/* Any error delivered by callback */
    }

    if ( gca_callback  &&  rcb->svc_parms->gca_cl_completion )  
    {
	GCA_DEBUG_MACRO(4)("%04d     GCR making GCA callback: status = 0x%x\n", 
			   rcb->assoc_id, rcb->svc_parms->status);
	(*rcb->svc_parms->gca_cl_completion)( (PTR)rcb->svc_parms->closure );
	if ( down )  status = OK;	/* Any error delivered by callback */
    }

    return( status );
}


/*
** Name: gcr_eval
**
** Description:
**	Determines input event for embedded Comm Server 
**	interface State Machine from the current request 
**	information.
** 
** Input:
**	rcb		Router control block.
**	request		Router request type.
**	down		TRUE if GCA request, FALSE if GCC request.
**
** Ouput:
**	None.
**
** Returns:
**	i4		Input event.
**
** History:
**	23-Oct-95 (gordy)
**	    Created.
*/

static i4
gcr_eval( GCR_CB *rcb, i4  request, bool down )
{
    i4  input = GCR_I_UNKNOWN;


    if ( down )
    {
	/*
	** Determine input event for GCA request.
	*/
	switch( request )
	{
	    case GCR_REQUEST :
		input = GCR_I_RQST;
		break;
	
	    /*
	    ** GCR_SEND is used for several different actions
	    ** by GCA which must be distinguished by the router.
	    ** The input event is dependent on the current state 
	    ** and various pieces of GCA info.
	    */
	    case GCR_SEND :

		/*
		** A Fast Select sends the message following the
		** peer info and (optionally) the security label
		** but only if a flag was set in the peer info.
		** Note that if we are in state GCR_S_SPEER then
		** we checked above for a security label and
		** decided there wasn't one to process.
		*/

		if ( ( rcb->state == GCR_S_SPEER )  &&
			   rcb->peer.gca_flags & GCA_LS_FASTSELECT )
		    input = GCR_I_FS;

		/*
		** Sends and receives are the expected 
		** actions while connected.
		*/
		else if( rcb->state == GCR_S_RQST )
		    input = GCR_I_SPEER;
		else if ( rcb->state == GCR_S_CONN ) 
		    input = GCR_I_SEND;

		/*
		** All other cases are considered invalid.
		*/
		else  input = GCR_I_INVALID;
		break;

	    case GCR_RECEIVE :
		if( rcb->state == GCR_S_SPEER ||
			rcb->state == GCR_S_FS )
		    input = GCR_I_RPEER;
		else if ( rcb->state == GCR_S_CONN )
		    input = GCR_I_RECEIVE;
		break;

	    case GCR_PURGE :
		input = GCR_I_PURGE;
		break;

	    case GCR_DISCONN :
		input = GCR_I_DISCONN;
		break;

	} /* switch */
    }
    else /* down */
    {
	/*
	** Determine input event for GCC request.
	*/
	switch( request )
	{

	    case GCR_RQRESP :
		input = GCR_I_RQRESP;
		break;

	    case GCR_SEND :
		input = GCR_I_DATA;
		break;

	    case GCR_RECEIVE :
		input = GCR_I_NEED;
		break;

	    case GCR_DISCONN :
		input = GCR_I_ABORT;
		break;

	} /* switch */
    } /* ! down */

    return( input );
}


/*
** Name: gcr_init_gcc
**
** Description:
**	Initialize the embedded Comm Server.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	STATUS
**
** History:
**	23-Oct-95 (gordy)
**	    Created.
**	10-oct-1996 (canor01)
**	    Use SystemCfgPrefix to allow system variable override.
*/

static STATUS
gcr_init_gcc( VOID )
{
    STATUS	(*call_list[5])();
    STATUS	generic_status = OK;
    CL_ERR_DESC	system_status;
    i4		count;
    char	*pm_default[ PM_MAX_ELEM ];

    MEfill( sizeof( system_status ), 0, (PTR)&system_status );

    /*
    ** Comm Server error logging is usually initialized
    ** with the Comm Server listen address.  Since the
    ** embedded Comm Server does not call GCA_LISTEN,
    ** we'll hard code our own indicator.
    */
    gcc_er_init( ERx( "GCC" ), ERx( "embedded" ) );

    /*
    ** Comm Server code expects its own set of PM defaults,
    ** which are usually set up in main() of the standalone
    ** Comm Server.  Since we are running as a sub-system,
    ** there is no telling what defaults have been set.  Save
    ** the current defaults and set the defaults expected.
    */
    for( count = 0; count < PM_MAX_ELEM; count++ )
	pm_default[ count ] = PMgetDefault( count );

    PMsetDefault( 0, SystemCfgPrefix );	/* Comm Server defaults */
    PMsetDefault( 1, PMhost() );
    PMsetDefault( 2, ERx( "gcc" ) );
    PMsetDefault( 3, ERx( "*" ) );

    /* Clear remaining defaults */
    for( count = 4; count < PM_MAX_ELEM; count++ )
	PMsetDefault( count, NULL );

    /*
    ** Initialize the Comm Server embedded interface.
    */
    if ( gcc_init( FALSE, CI_INGRES_NET, 0, NULL,
                   &generic_status, &system_status ) != OK )
    {
	if ( generic_status == OK )  generic_status = E_GC2005_INIT_FAIL;
	goto complete;
    }

    call_list[0] = GCcinit;
    call_list[1] = gcc_alinit;
    call_list[2] = gcc_plinit;
    call_list[3] = gcc_slinit;
    call_list[4] = gcc_tlinit;
    count = 5;

    if ( gcc_call( &count, call_list, &generic_status, &system_status ) != OK )
    {
	STATUS status;

	if ( generic_status == OK )  generic_status = E_GC2005_INIT_FAIL;

	/*
	** Terminate those layers which have been initialized,
	** count holds the number of successful initializations.
	*/
	call_list[0] = gcc_tlterm;
	call_list[1] = gcc_slterm;
	call_list[2] = gcc_plterm;
	call_list[3] = gcc_alterm;
	call_list[4] = GCcterm;

	gcc_call( &count, &call_list[ 5 - count ], 
		  &status, &system_status );
	gcc_term( &status, &system_status );
	goto complete;
    }

    /*
    ** The standalone Comm Server saves the GCA header length 
    ** after calling GCA_INITIATE.  Since we have bypassed 
    ** this call, we need to set the header length in the  
    ** GCC global data area.  At API level 5, headers are no
    ** longer passed across the GCA interface.
    */
    IIGCc_global->gcc_gca_hdr_lng = 0;

  complete:

    /*
    ** Reset original PM defaults.
    */
    for( count = 0; count < PM_MAX_ELEM; count++ )
	PMsetDefault( count, pm_default[ count ] );


    return( generic_status );
}



/*
** Name: gcr_term_gcc
**
** Description:
**	Terminate the embedded Comm Server.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	STATUS
**
** History:
**	23-Oct-95 (gordy)
**	    Created.
*/

STATUS
gcr_term_gcc( VOID )
{
    STATUS	(*call_list[5])();
    STATUS	generic_status = OK;
    CL_ERR_DESC	system_status;
    i4		count;

    MEfill( sizeof( system_status ), 0, (PTR)&system_status );

    call_list[0] = gcc_tlterm;
    call_list[1] = gcc_slterm;
    call_list[2] = gcc_plterm;
    call_list[3] = gcc_alterm;
    call_list[4] = GCcterm;
    count = 5;

    gcc_call( &count, call_list, &generic_status, &system_status );
    gcc_term( &generic_status, &system_status );
    return( OK );
}


/*
** Name: gcr_peer_info
**
** Description:
**	Save peer info for connection request.
**
** Input:
**	rcb		Router control block.
**
** Output:
**	None.
**
** Returns:
**	STATUS
**
** History:
**	23-Oct-95 (gordy)
**	    Created.
**	 2-Jul-99 (rajus01)
**	    Copy the aux_data in rcb.
*/

static STATUS
gcr_peer_info( GCR_CB *rcb )
{
    i4 		aux_len;


    MEcopy( rcb->svc_parms->svc_buffer, 
	    sizeof( GCA_PEER_INFO ), (PTR)&rcb->peer );

    rcb->svc_parms->svc_buffer += rcb->peer.gca_length;
    aux_len	= rcb->svc_parms->svc_send_length - rcb->peer.gca_length;

    if( ! ( rcb->aux_data = gca_alloc( aux_len ) ) )
	 return( E_GC0013_ASSFL_MEM );

    MEcopy( (PTR)rcb->svc_parms->svc_buffer, aux_len, (PTR)rcb->aux_data );  

    /*
    ** Adjust peer info as is done for GCA_LISTEN in the
    ** LS_CHK_PEER action.
    */
    rcb->peer.gca_partner_protocol = 
		min( rcb->peer.gca_partner_protocol, GCC_GCA_PROTOCOL_LEVEL );

    if ( ! rcb->peer.gca_user_id[0] )
    {
	char *ptr = rcb->peer.gca_user_id;
	IDname( &ptr );
	if ( ptr != rcb->peer.gca_user_id )  STcopy(ptr, rcb->peer.gca_user_id);
    }

    return( OK );
}



/*
** Name: gcr_fs_info
**
** Description:
**	Save fast select info for connection request.
**
** Input:
**	rcb		Router control block.
**
** Output:
**	None.
**
** Returns:
**	STATUS
**
** History:
**	23-Oct-95 (gordy)
**	    Created.
*/

static STATUS
gcr_fs_info( GCR_CB *rcb )
{
    STATUS status = OK;

    rcb->fs_data = gca_alloc( rcb->svc_parms->svc_send_length );

    if ( ! rcb->fs_data )
	status = E_GC0013_ASSFL_MEM;
    else
    {
	rcb->flags |= GCR_F_FS;
	rcb->fs_len = (u_i2)rcb->svc_parms->svc_send_length;
	MEcopy(rcb->svc_parms->svc_buffer, rcb->fs_len, (char *)rcb->fs_data);
    }

    return( status );
}


/*
** Name: gcr_request
**
** Description:
**	Prepare to call GCC with a connection request.  This 
**	routine builds the GCC data structures to simulate 
**	what is done in gcc_alinit() and the AALSN action in 
**	gcc_al().  Since the work done in gcc_gca_exit() for 
**	a completed listen is fairly complex, we will use
**	gcc_gca_exit() as our entry point into the embedded
**	Comm Server (as a callback by the calling routine).
**
**	1) Allocate GCC_MDE, initialize with GCC_A_LISTEN, A_ASSOCIATE, RQS
**	2) Allocate GCC_CCB, initialize with SACLD
**	3) Initialize GCA_LS_PARMS in mde with GCA info in rcb
**
** Input:
**	rcb		Router control block.
**
** Output:
**	None.
**
** Returns:
**	STATUS
**
** History:
**	23-Oct-95 (gordy)
**	    Created.
**	18-Jan-96 (gordy)
**	    Since we are initiating a callback to the Comm Server, set
**	    the rcb parmlist which would have been set if the Comm
**	    Server had called us.
**	 2-Jul-99 (rajus01)
**	    Process the aux_data in rcb before assigning it in
**	    ls_parms->gca_aux_data.
*/

static STATUS
gcr_request( GCR_CB *rcb )
{
    GCC_MDE		*mde;
    GCC_CCB		*ccb;
    GCA_LS_PARMS	*ls_parms;
    i4			count;
    GCA_AUX_DATA	hdr;
    PTR			data = rcb->aux_data;
    bool		valid_type = FALSE;


    /*
    ** Allocate the CCB for this new connection.  This
    ** will be freed when the async_cnt drops to 0.
    ** Normally, async_cnt is bumped by AL or TL.  We
    ** also bump async_cnt to check for successful
    ** completion of AL and TL at GCR_RELEASE time.
    ** Note that we bump async_cnt twice below, once
    ** for us and once for AL.
    */
    if ( ! (ccb = gcc_add_ccb()) )
	return( E_GC0013_ASSFL_MEM );

    /*
    ** Allocate the MDE for this GCC request.  This
    ** will be freed by the embedded Comm Server as
    ** a result of its normal processing.
    */
    if ( ! (mde = (GCC_MDE *)gcc_add_mde( ccb, 0 )) )
    {
	gcc_del_ccb( ccb );
	return( E_GC0013_ASSFL_MEM );
    }

    rcb->ccb = (PTR)ccb;
    rcb->mde = (PTR)mde;

    ccb->ccb_aae.a_state = SACLD;
    ccb->ccb_hdr.async_cnt += 2;
    ccb->ccb_aae.a_version = AL_PROTOCOL_VRSN;

    mde->ccb = ccb;
    mde->service_primitive = A_ASSOCIATE;
    mde->primitive_type = RQS;
    mde->p1 = mde->p2 = mde->papp;
    mde->gca_rq_type = GCC_A_LISTEN;

    ls_parms = (GCA_LS_PARMS *)&mde->mde_act_parms.gca_parms;
    rcb->parmlist = (GCA_PARMLIST *)ls_parms;
    ls_parms->gca_status = OK;

    ls_parms->gca_assoc_id = rcb->assoc_id;

    /*
    ** The following parameters are generally returned
    ** from GClisten().  We do our best to provide
    ** something reasonable.
    */
    ls_parms->gca_size_advise = GCA_NORM_SIZE;
    ls_parms->gca_account_name = rcb->peer.gca_user_id;
    ls_parms->gca_access_point_identifier = ERx( "embedded" );

    /*
    ** Copy peer info to listen parameters.
    */
    ls_parms->gca_partner_protocol = rcb->peer.gca_partner_protocol;
    ls_parms->gca_flags = rcb->peer.gca_flags;
    ls_parms->gca_user_name = rcb->peer.gca_user_id;
    ls_parms->gca_password = NULL;

    for ( count = rcb->peer.gca_aux_count; !valid_type && count; count-- )
    {
	MEcopy( data, sizeof( hdr ), (PTR)&hdr );

	switch( hdr.type_aux_data )
	{
	    case GCA_ID_SHUTDOWN:
	    case GCA_ID_QUIESCE:
	    case GCA_ID_RMT_INFO:
		valid_type = TRUE;
		MEcopy( data, hdr.len_aux_data, rcb->aux_data ); 
		ls_parms->gca_aux_data = rcb->aux_data; 
    		ls_parms->gca_l_aux_data = hdr.len_aux_data; 
		break;
	    default:
	        data += hdr.len_aux_data;
	    	break;
	}

	if( valid_type )
	   break;
    }

    if ( rcb->flags & GCR_F_FS )
    {
	/*
	** At API level 5, remote fast select info
	** is returned in the listen parms, and no
	** message header is present.
	*/
	ls_parms->gca_fs_data = rcb->fs_data + sizeof( GCA_MSG_HDR );
	ls_parms->gca_l_fs_data = rcb->fs_len - sizeof( GCA_MSG_HDR );
	ls_parms->gca_message_type = ((GCA_MSG_HDR *)rcb->fs_data)->msg_type;
    }

    /*
    ** Security labels are passed up from the CL,
    ** which we can't duplicate.  Security labels
    ** may also be passed in the peer info, but
    ** GCA_LISTEN only uses them if the partner is 
    ** trusted, which is also determined by the CL.  
    ** At this point, there does not appear to be 
    ** a way to provide the security label.
    */
    ls_parms->gca_sec_label = NULL;

    return( OK );
}


/*
** Name: gcr_response
**
** Description:
**	Service a GCA_RQRESP request from the embedded 
**	Comm Server by building the peer info to be 
**	returned to GCA's receive peer request.
**
** Input:
**	rcb		Router control block.
**
** Output:
**	None.
**
** Returns:
**	STATUS
**
** History:
**	23-Oct-95 (gordy)
**	    Created.
**	 2-Jul-99 (rajus01)
**	    Build the new GCA_PEER_INFO_INFO structure.
*/

static STATUS
gcr_response( GCR_CB *rcb )
{
    GCA_RR_PARMS	*rr_parms = (GCA_RR_PARMS *)rcb->parmlist;
    u_i4		old_flags = rcb->peer.gca_flags;

    /*
    ** Build the peer info, as is done in LS_RRINIT 
    ** and LS_SEND_PEER and return to GCA.  If the
    ** connection request failed, we signal that an
    ** orderly shutdown is in progress.
    */
    if ( rr_parms->gca_request_status != OK )
	rcb->flags |= GCR_F_RELEASE;

    rcb->peer.gca_length = sizeof( GCA_PEER_INFO );
    rcb->peer.gca_id = GCA_PEER_ID;
    rcb->peer.gca_flags = 0;
    rcb->peer.gca_aux_count = 0;
    rcb->peer.gca_status = rr_parms->gca_request_status;
    rcb->peer.gca_partner_protocol = rr_parms->gca_local_protocol;
    MEfill(sizeof(rcb->peer.gca_user_id), EOS, (PTR)rcb->peer.gca_user_id);

    if ( rr_parms->gca_modifiers & GCA_RQ_DESCR || old_flags & GCA_DESCR_RQD )
	rcb->peer.gca_flags |= GCA_DESCR_RQD;

    rcb->svc_parms->status = OK;

    rcb->svc_parms->rcv_data_length =
   		 min( rcb->svc_parms->reqd_amount, sizeof( rcb->peer) );

    MEcopy( (PTR)&rcb->peer, rcb->svc_parms->rcv_data_length,
			    rcb->svc_parms->svc_buffer );


    return( OK );
}


/*
** Name: gcr_send
**
** Description:
**
** Input:
**	rcb		Router control block.
**	subchan		Normal or Expedited.
**
** Output:
**	gca_done	TRUE if GCA request satisfied.
**	gcc_done	TRUE if GCC request satisfied.
**
** Returns:
**	STATUS
**
** History:
**	23-Oct-95 (gordy)
**	    Created.
*/

static STATUS
gcr_send(GCR_CB *rcb, i4  subchan, bool *gca_done, bool *gcc_done)
{
    GCR_SR_INFO	*send = &rcb->send[ subchan ];
    i4		len;

    *gca_done = FALSE;
    *gcc_done = FALSE;


    /*
    ** Check to see if we are at the start of a message.
    */
    if ( ! (send->flags & GCR_F_CONT) )
    {
	/*
	** Save the message header.  Note that we 
	** may have already read part of the header.
	*/
	len = sizeof( GCA_MSG_HDR ) - send->hdr_len;

	if ( len > send->i_buf_len )
	{
	    /*
	    ** There is not enough input to complete
	    ** the message header.  We must read the 
	    ** partial header and allow GCA to send 
	    ** the remainder.
	    */
	    MEcopy( (PTR)send->i_buf_ptr, send->i_buf_len, 
		    (PTR)(((char *)&send->hdr) + send->hdr_len) );
	    send->hdr_len += send->i_buf_len;

	    /*
	    ** We have completed the GCA request.  Turn off 
	    ** the GCA data available indicator.  Be sure to 
	    ** reset the GCA request info in the rcb, in case 
	    ** we were called as a result of a GCC request.
	    */
	    send->flags &= ~GCR_F_GCA;
	    send->svc_parms->status = OK;
	    rcb->svc_parms = send->svc_parms;
	    *gca_done = TRUE;

	    return( OK );
	}
	
	MEcopy( (PTR)send->i_buf_ptr, len, 
		(PTR)(((char *)&send->hdr) + send->hdr_len) );
	send->hdr_len = 0;
	send->i_buf_ptr += len;
	send->i_buf_len -= len;
	send->i_msg_len = send->hdr.data_length;
	send->end_of_data = send->hdr.flags.end_of_data;
    }


    /*
    ** Copy the next piece of the message.  There
    ** may be another message following the current
    ** one in the buffer, in which case i_msg_len
    ** will be less than i_buf_len.  We may be on
    ** the last (or only) message in the buffer, 
    ** in which case i_msg_len will be greater than
    ** (if message split) or equal to i_buf_len.
    ** We must also limit the size to what space
    ** remains in the output buffer.
    */
    len = min( min( send->i_msg_len, send->i_buf_len ), send->o_buf_len );

    if ( len )
    {
	MEcopy( (PTR)send->i_buf_ptr, len, (PTR)send->o_buf_ptr );
	send->i_buf_ptr += len;
	send->i_buf_len -= len;
	send->i_msg_len -= len;
	send->o_buf_ptr += len;
	send->o_buf_len -= len;
	send->o_msg_len += len;
    }

    /*
    ** We need to send the output buffer if the current
    ** message is done or the output buffer is full.
    */
    if ( ! send->i_msg_len  ||  ! send->o_buf_len )
    {
	GCA_RV_PARMS	*rv_parms = (GCA_RV_PARMS *)send->parmlist;

	/*
	** Build the message header if expected by GCC.
	*/
	if ( IIGCc_global->gcc_gca_hdr_lng )
	{
	    /*
	    ** Adjust header fields which may change
	    ** because of differences in send/receive
	    ** buffers.
	    */
	    send->hdr.data_length = send->o_msg_len;
	    send->hdr.data_offset = IIGCc_global->gcc_gca_hdr_lng;
	    send->hdr.flags.end_of_data = 
				    (send->end_of_data  &&  ! send->i_msg_len);
	    MEcopy( (PTR)&send->hdr, 
		    IIGCc_global->gcc_gca_hdr_lng, (PTR)rv_parms->gca_buffer );
	}
	
	/*
	** We have completed the GCC request.  Turn off 
	** the GCC waiting indicator.  Be sure to reset
	** the GCC request info in the rcb, in case we
	** were called as a result of a GCA request.  We 
	** flag the processing of a GCA_RELEASE message 
	** to indicate that a controlled shutdown is in 
	** progress (rather than an abort).
	*/
	send->flags &= ~GCR_F_GCC;
	rv_parms->gca_status = OK;
	rv_parms->gca_message_type = send->hdr.msg_type;
	rv_parms->gca_d_length = send->o_msg_len;
	rv_parms->gca_data_area = rv_parms->gca_buffer + 
				  IIGCc_global->gcc_gca_hdr_lng;
	rv_parms->gca_end_of_group = send->hdr.flags.end_of_group;
	rv_parms->gca_end_of_data = (send->end_of_data  &&  ! send->i_msg_len);

	/*
	** Check for special processing based on message type.
	*/
	if ( ! send->i_msg_len )
	    if ( rv_parms->gca_message_type == GCA_RELEASE )
		rcb->flags |= GCR_F_RELEASE;
	    else  if ( rv_parms->gca_message_type == GCA_ATTENTION  )
	    {
		/*
		** Receiving a GCA_ATTENTION message puts GCA on
		** the Comm Server side in the rpurging state (see 
		** RV_DONE).  We should reject any GCC requests to 
		** send messages other than GCA_IACK (see SD_CKIACK).  
		** We must also simulate a call to GCpurge() by the 
		** Comm Server GCA by aborting the next (or current) 
		** GCA receive request with the new_chop indicator 
		** (see GC_RECVPRG and GC_RECVCMP).
		*/
		GCR_SR_INFO	*recv = &rcb->recv[ GCA_NORMAL ];

		/*
		** Set indicator to reject GCC sends until GCA_IACK.
		*/
		recv->flags |= GCR_F_IACK;

		/*
		** If GCA is not currently waiting for data, flag 
		** the need to purge the next GCA receive request.
		** Just to be safe, only purge at the end of any
		** continued message.
		*/
		if ( recv->flags & GCR_F_CONT || ! (recv->flags & GCR_F_GCA) )
		    recv->flags |= GCR_F_PURGE;
		else
		{
		    /*
		    ** Complete the GCA receive request 
		    ** with a new_chop indicator.
		    */
		    GCA_DEBUG_MACRO(4)( "%04d     GCR purging GCA request.\n", 
					rcb->assoc_id);
		    recv->flags &= ~GCR_F_GCA;
		    recv->svc_parms->flags.new_chop = 1;
		    recv->svc_parms->rcv_data_length = 0;
		    recv->svc_parms->status = OK;

		    GCA_DEBUG_MACRO(4)( "%04d     GCR making GCA callback: status = 0x%x\n", 
					rcb->assoc_id, 
					recv->svc_parms->status );
		    (*recv->svc_parms->gca_cl_completion)((PTR)recv->svc_parms);
		}
	    }

	rcb->parmlist = send->parmlist;
	rcb->mde = send->mde;
	*gcc_done = TRUE;
    }

    /*
    ** Check to see if we have reached the end of the current message.
    ** If so, the next thing we expect to see is a GCA message header.
    ** Note that this is independent of the state of the GCA request
    ** since the message may be split into multiple GCA requests.
    */
    if ( send->i_msg_len )  
	send->flags |= GCR_F_CONT;
    else
	send->flags &= ~GCR_F_CONT;

    if ( ! send->i_buf_len )
    {
	/*
	** We have completed the GCA request.  Turn off 
	** the GCA data available indicator.  Be sure to 
	** reset the GCA request info in the rcb, in case 
	** we were called as a result of a GCC request.
	*/
	send->flags &= ~GCR_F_GCA;
	send->svc_parms->status = OK;
	rcb->svc_parms = send->svc_parms;
	*gca_done = TRUE;
    }

    return( OK );
}


/*
** Name: gcr_receive
**
** Description:
**
** Input:
**	rcb		Router control block.
**	subchan		Normal or Expedited.
**
** Output:
**	gca_done	TRUE if GCA request satisfied.
**	gcc_done	TRUE if GCC request satisfied.
**
** Returns:
**	STATUS
**
** History:
**	23-Oct-95 (gordy)
**	    Created.
*/

static STATUS
gcr_receive( GCR_CB *rcb, i4  subchan, bool *gca_done, bool *gcc_done )
{
    GCR_SR_INFO		*recv = &rcb->recv[ subchan ]; 
    i4			len;

    *gca_done = FALSE;
    *gcc_done = FALSE;

    /*
    ** The GCA message header is only sent at the
    ** start of the data.  If this is a continued
    ** GCC request, we skip the message header and
    ** send what data remains.
    */
    if ( ! (recv->flags & GCR_F_CONT) )
    {
	GCA_SD_PARMS	*sd_parms = (GCA_SD_PARMS *)recv->parmlist;
	GCA_MSG_HDR	hdr;

	/*
	** We had better have room to send the GCA
	** message header.
	*/
	if ( (len = sizeof( GCA_MSG_HDR )) > recv->o_buf_len )
	{
	    /*
	    ** Fail the GCA request.  Be sure to reset
	    ** the GCA request info in the rcb, in case
	    ** we were called as a result of a GCC request.
	    */
	    *gca_done = TRUE;
	    recv->flags &= ~GCR_F_GCA;
	    rcb->svc_parms = recv->svc_parms;
	    rcb->svc_parms->status = E_GC0010_BUF_TOO_SMALL;
	    return( E_GC0010_BUF_TOO_SMALL );
	}

	/*
	** We flag the processing of a GCA_RELEASE message 
	** to indicate that a controlled shutdown is in 
	** progress (rather than an abort).
	*/
	if ( sd_parms->gca_message_type == GCA_RELEASE )
	    rcb->flags |= GCR_F_RELEASE;

	/*
	** Since GCA expects to read a GCA message header,
	** we must build it here.  
	*/
	hdr.buffer_length = recv->o_buf_len;
	hdr.msg_type = sd_parms->gca_message_type;
	hdr.data_length = sd_parms->gca_msg_length;
	hdr.data_offset = len;
	hdr.flags.flow_indicator = subchan;
	hdr.flags.end_of_data = sd_parms->gca_end_of_data;
	hdr.flags.end_of_group = gca_is_eog( sd_parms->gca_message_type,
					     rcb->peer.gca_partner_protocol );
	MEcopy( (PTR)&hdr, len, (PTR)recv->o_buf_ptr );
	recv->o_buf_ptr += len;
	recv->o_buf_len -= len;
	recv->o_msg_len += len;
    }

    /*
    ** Copy as much of the GCC data as possible
    ** into the GCA buffer.
    */
    len = min( recv->i_buf_len, recv->o_buf_len );

    if ( len )
    {
	MEcopy( (PTR)recv->i_buf_ptr, len, (PTR)recv->o_buf_ptr );
	recv->i_buf_ptr += len;
	recv->i_buf_len -= len;
	recv->o_msg_len += len;
    }

    if ( recv->i_buf_len )  
    {
	/*
	** There is still GCC data to be sent to GCA.  
	** Set the continue indicator.  Note that the 
	** GCC data available indicator should already 
	** be set.
	*/
	recv->flags |= GCR_F_CONT;
    }
    else
    {
	/*
	** We have completed the GCC request.  Turn off 
	** the GCC data available indicator.  Be sure to
	** reset the GCC request info in the rcb, in case
	** we were called as a result of a GCA request.
	*/
	recv->flags &= ~(GCR_F_GCC | GCR_F_CONT);
	((GCA_ALL_PARMS *)recv->parmlist)->gca_status = OK;
	rcb->parmlist = recv->parmlist;
	rcb->mde = recv->mde;
        *gcc_done = TRUE;
    }

    /*
    ** We have completed the GCA request, even if the 
    ** GCC request is continued.  Turn off the GCA 
    ** waiting indicator.  Be sure to reset the GCA 
    ** request info in the rcb, in case we were called
    ** as a result of a GCC request.
    */
    recv->flags &= ~GCR_F_GCA;
    recv->svc_parms->rcv_data_length = recv->o_msg_len;
    recv->svc_parms->flags.new_chop = 0;
    recv->svc_parms->status = OK;
    rcb->svc_parms = recv->svc_parms;
    *gca_done = TRUE;

    return( OK );
}


/*
** Name: gcr_abort
**
** Description:
**
** Input:
**	rcb		Router control block.
**	flags		GCR_F_GCA and/or GCR_F_GCC.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	23-Oct-95 (gordy)
**	    Created.
*/

static VOID
gcr_abort( GCR_CB *rcb, i4  flags )
{
    i4  i;

    for( i = GCA_NORMAL; i <= GCA_EXPEDITED; i++ )
    {
	if ( flags & GCR_F_GCC  &&  rcb->send[ i ].flags & GCR_F_GCC )
	{
	    GCA_DEBUG_MACRO(4)( "%04d     GCR aborting GCC receive request\n", 
				rcb->assoc_id);
	    rcb->send[ i ].flags &= ~(GCR_F_GCC | GCR_F_CONT);
	    ((GCA_ALL_PARMS *)rcb->send[ i ].parmlist)->gca_status = 
							E_GC0023_ASSOC_RLSED;
	    GCA_DEBUG_MACRO(4)( "%04d     GCR making GCC callback: status = 0x%x\n",
				rcb->assoc_id,
				((GCA_ALL_PARMS *)
				 rcb->send[ i ].parmlist)->gca_status );
	    gcc_gca_exit( rcb->send[ i ].mde );
	}

	if ( flags & GCR_F_GCC  &&  rcb->recv[ i ].flags & GCR_F_GCC )
	{
	    GCA_DEBUG_MACRO(4)( "%04d     GCR aborting GCC send request\n", 
				rcb->assoc_id );
	    rcb->recv[ i ].flags &= ~(GCR_F_GCC | GCR_F_CONT);
	    ((GCA_ALL_PARMS *)rcb->recv[ i ].parmlist)->gca_status = 
							E_GC0023_ASSOC_RLSED;
	    GCA_DEBUG_MACRO(4)( "%04d     GCR making GCC callback: status = 0x%x\n",
				rcb->assoc_id,
				((GCA_ALL_PARMS *)
				 rcb->recv[ i ].parmlist)->gca_status );
	    gcc_gca_exit( rcb->recv[ i ].mde );
	}

	if ( flags & GCR_F_GCA  &&  rcb->send[ i ].flags & GCR_F_GCA )
	{
	    GCA_DEBUG_MACRO(4)( "%04d     GCR aborting GCA send request\n", 
				rcb->assoc_id );
	    rcb->send[ i ].flags &= ~GCR_F_GCA;
	    rcb->send[ i ].svc_parms->status = E_GC0023_ASSOC_RLSED;

	    GCA_DEBUG_MACRO(4)( "%04d     GCR making GCA callback: status = 0x%x\n", 
				rcb->assoc_id, 
				rcb->send[ i ].svc_parms->status );
	    (*rcb->send[ i ].svc_parms->gca_cl_completion)
					    ( (PTR)rcb->send[ i ].svc_parms );
	}

	if ( flags & GCR_F_GCA  &&  rcb->recv[ i ].flags & GCR_F_GCA )
	{
	    GCA_DEBUG_MACRO(4)( "%04d     GCR aborting GCA receive request\n", 
				rcb->assoc_id);
	    rcb->recv[ i ].flags &= ~GCR_F_GCA;
	    rcb->recv[ i ].svc_parms->status = E_GC0023_ASSOC_RLSED;

	    GCA_DEBUG_MACRO(4)( "%04d     GCR making GCA callback: status = 0x%x\n", 
				rcb->assoc_id, 
				rcb->recv[ i ].svc_parms->status );
	    (*rcb->recv[ i ].svc_parms->gca_cl_completion)
					    ( (PTR)rcb->recv[ i ].svc_parms );
	}
    }

    return;
}

#endif /* GCF_EMBEDDED_GCC */
