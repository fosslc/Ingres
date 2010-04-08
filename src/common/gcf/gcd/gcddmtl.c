/*
** Copyright (c) 2004, 2007 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <me.h>
#include <st.h>
#include <tr.h>

#include <iicommon.h>
#include <gcu.h>
#include <gcdint.h>
#include <gcdmsg.h>

/*
** Name: gcddmtl.c
**
** Description:
**	GCD TL Service Provider for the DAM-TL protocol.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	10-Sep-99 (gordy)
**	    Parameterized initial TL connection info.
**	14-Sep-99 (gordy)
**	    Implemented JDBC error messages.
**	18-Oct-99 (gordy)
**	    Added flow control to GCC/MSG sub-systems.
**	 4-Nov-99 (gordy)
**	    Added support for handling TL interrupts.
**	16-Nov-99 (gordy)
**	    Skip unknown connection parameters.
**	13-Dec-99 (gordy)
**	    Negotiate GCC parameters prior to session start
**	    so that values are available to session layer.
**	18-Dec-02 (gordy)
**	    Converted for Data Access Server (GCD).
**	20-Dec-02 (gordy)
**	    Packet header ID now protocol dependent.
**	15-Jan-03 (gordy)
**	    Abstracted TL packet processing with Service Provider
**	    interface and separated JDBC and DAM TL implementations.
**	16-Jan-03 (gordy)
**	    Send the MSG layer ID along with the MSG layer info.
**      07-Mar_2003(wansh01)
**          Rename shutdown() to dmtl_shutdown() to avoid redefination with
**          winsock shutdown().
**	 5-Dec-07 (gordy)
**	    Added accept() entry point to GCC Service Provider.
*/	

/*
** Forward references.
*/

static	bool	dmtl_accept( GCC_SRVC *, GCD_RCB * );
static	void	dmtl_snd_xoff( GCD_CCB *, bool );
static	void	dmtl_data( GCD_RCB * );
static	void	dmtl_abort( GCD_CCB * );
static	void	dmtl_rcv_xoff( GCD_CCB *, bool );
static	void	dmtl_send( GCD_RCB * );
static	void	dmtl_disc( GCD_CCB *, STATUS );
static	STATUS	read_cr_parms( GCD_RCB *, u_i1 *, u_i1 **, u_i2 * );
static	void	build_cc_parms( GCD_RCB *, u_i1, u_i1 *, u_i2 );
static	void	dmtl_shutdown( GCD_CCB *, GCD_RCB *, STATUS );
static	void	send_dr( GCD_CCB *, GCD_RCB *, STATUS );
static	u_i2	read_hdr( GCD_RCB *, DAM_TL_HDR * );
static	void	build_hdr( GCD_RCB *, DAM_TL_HDR * );

/*
** TL Service Provider definitions for GCC & MSG Services.
*/

GLOBALDEF TL_SRVC	gcd_dmtl_service =
{
    dmtl_accept,
    dmtl_snd_xoff,
    dmtl_data,
    dmtl_abort,
};


static TL_MSG_SRVC	dmtl_msg_service = 
{
    dmtl_rcv_xoff,
    dmtl_send,
    dmtl_disc,
};


/*
** Name: dmtl_accept
**
** Description:
**	Called for first data packet received on a connection 
**	to determine which TL Service will service the connection.
**	Returns FALSE if provider does not service the request.
**	Returns TRUE otherwise.  Provider may accept or reject
**	connections which it services.
**
**	If TRUE is returned, the RCB is consumed.  Otherwise,
**	the RCB is not changed.
**
**	If the Service Provider would normally accept the
**	connection but an error occurs, the provider still
**	indicates that the connection is serviced by this
**	provider and initiates whatever actions are needed to
**	abort the connection.  This must ultimately result in
**	a call to the GCC Service Provider disc() function.
**
** Input:
**	GCC_SRVC *	The GCC Service servicing the connection.
**	GCD_RCB *	Request control block containing TL packet data.
**
** Output:
**	None.
**
** Returns:
**	bool		TRUE if Service Provider accepts connection.
**
** History:
**	15-Jan-03 (gordy)
**	    Created.
**	 5-Dec-07 (gordy)
**	    Added accept() entry point to GCC Service Provider to allow
**	    GCC providers to reject a connection an provide an error code.
*/

static bool
dmtl_accept( GCC_SRVC *gcc_service, GCD_RCB *rcb )
{
    GCD_CCB	*ccb = rcb->ccb;
    DAM_TL_HDR	hdr;
    STATUS	status = OK;
    u_i1	*rcb_ptr;
    u_i4	rcb_len;
    u_i1	*msg_parms = NULL;
    u_i2	msg_len = 0;
    u_i1	size = DAM_TL_PKT_MIN;

    if ( GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d    GCD DMTL testing new connection\n", ccb->id );

    /*
    ** Save RCB settings in case we don't accept
    ** the connection and need to restore the RCB.
    */
    rcb_ptr = rcb->buf_ptr;
    rcb_len = rcb->buf_len;

    /*
    ** Validate the packet header and request type.
    ** If invalid, refuse GCC request.  We accept
    ** both the JDBC TL and DAM-TL packet ID's for
    ** now.  The protocol level parameter will tell
    ** which TL should handle this request.
    */
    read_hdr( rcb, &hdr );
    if ( (hdr.tl_id != DAM_TL_ID_1  &&  hdr.tl_id != DAM_TL_ID_2)  ||  
	 hdr.msg_id != DAM_TL_CR )
    {
	rcb->buf_ptr = rcb_ptr;
	rcb->buf_len = rcb_len;
	return( FALSE );
    }

    /*
    ** Read the TL Connection Request parameters.
    **
    ** The DAM-TL protocol is a super-set of the JDBC TL
    ** protocol.  We should be able to read JDBC connect
    ** parms, so if there is an error we will accept the
    ** GCC connection and then abort the TL connection.
    **
    ** We refuse the connection if the protocol level and
    ** packet header ID indicate that this is for JDBC TL.
    */
    if ( (status = read_cr_parms( rcb, &size, &msg_parms, &msg_len )) == OK  &&
         hdr.tl_id == DAM_TL_ID_1 && ccb->tl.proto_lvl < DAM_TL_PROTO_2 )
    {
	rcb->buf_ptr = rcb_ptr;
	rcb->buf_len = rcb_len;
	return( FALSE );
    }

    /*
    ** We will service this connection from GCC,
    ** though we may detect an error and abort
    ** the client connection.
    **
    ** We must respond to this request with the
    ** same TL packet ID as the original request.
    ** Subsequent packets must have the packet
    ** ID appropriate for the protocol level.
    */
    if ( GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d    GCD DMTL servicing request\n", ccb->id );

    ccb->tl.gcc_service = gcc_service;
    ccb->tl.packet_id = hdr.tl_id;

    /*
    ** Validate connection parameters.
    */
    for( ; status == OK; )
    {
	if ( ccb->tl.proto_lvl < DAM_TL_PROTO_1  ||  size < DAM_TL_PKT_MIN )
	{
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD invalid param value: (%d,%d)\n",
			   ccb->id, ccb->tl.proto_lvl, size );
	    status = E_GC480A_PROTOCOL_ERROR;
	    break;
	}

	/*
	** Negotiate connection parameters.
	*/
	if ( ccb->tl.proto_lvl > DAM_TL_DFLT_PROTO )
	    ccb->tl.proto_lvl = DAM_TL_DFLT_PROTO;
	if ( size > DAM_TL_PKT_DEF )  size = DAM_TL_PKT_DEF;
	ccb->max_buff_len = 1 << size;

	if ( GCD_global.gcd_trace_level >= 4 )
	{
	    TRdisplay( "%4d        protocol level: %d\n", 
		       ccb->id, ccb->tl.proto_lvl );
	    TRdisplay( "%4d        buffer size: %d\n", 
		       ccb->id, ccb->max_buff_len );
	}

	/*
	** Initialize our MSG Service Provider.
	*/
	if ( (status = (*gcd_dam_service.init)( &dmtl_msg_service, ccb, 
						&msg_parms, &msg_len )) != OK )
	    break;

	/*
	** OK, we are ready to accept the connection.  Inform the GCC
	** Service provider and verify that the connection should be
	** accepted.
	*/
	ccb->tl.msg_service = &gcd_dam_service;
	status = (*gcc_service->accept)( &gcd_dmtl_service, ccb );
	break;
    }

    if ( status != OK )
    {
	if ( GCD_global.gcd_trace_level >= 3 )
	    TRdisplay( "%4d    GCD DMTL rejecting connection: 0x%x\n", 
	    		ccb->id, status );

	dmtl_shutdown( ccb, rcb, status );
    }
    else
    {
	if ( GCD_global.gcd_trace_level >= 3 )
	    TRdisplay( "%4d    GCD DMTL accepting connection\n", ccb->id );

	/*
	** Build and send TL Connection Confirmation packet.
	** We use the same packet header ID as was passed in.
	*/
	gcd_init_rcb( rcb );
	build_cc_parms( rcb, size, msg_parms, msg_len );
	hdr.tl_id = ccb->tl.packet_id;
	hdr.msg_id = DAM_TL_CC;
	build_hdr( rcb, &hdr );
	(*ccb->tl.gcc_service->send)( rcb );
    }

    /*
    ** Subsequent communications must use the packet ID appropriate
    ** for the negotiated protocol level.
    **
    ** Whether the connection was accepted or rejected, we assumed
    ** responsibility for servicing the current request, so return
    ** the appropriate indication.
    */
    ccb->tl.packet_id = DAM_TL_ID_2;
    return( TRUE );
}


/*
** Name: dmtl_snd_xoff
**
** Description:
**	Control data flow being sent on a connection.
**
** Input:
**	GCD_CCB *	Connection control block.
**	bool		TRUE to pause sending data, FALSE to resume.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	15-Jan-03 (gordy)
**	    Created.
*/

static void 
dmtl_snd_xoff( GCD_CCB *ccb, bool xoff )
{
    if ( ccb->tl.msg_service )  (*ccb->tl.msg_service->xoff)( ccb, xoff );
    return;
}


/*
** Name: dmtl_data
**
** Description:
**	Called for each data packet received on a 
**	connection (except the first, see accept()).
**
** Input:
**	GCD_RCB *	Request control block containing TL packet data.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	15-Jan-03 (gordy)
**	    Created.
*/

static void 
dmtl_data( GCD_RCB *rcb )
{
    GCD_CCB	*ccb = rcb->ccb;
    DAM_TL_HDR	hdr;
    STATUS	status = E_GC480A_PROTOCOL_ERROR;
    u_i2	pkt_type;

    /*
    ** Breaking from the switch will shutdown the connection.
    ** If status is not changed, connection will be aborted
    ** with protocol error (set to OK for simple shutdown).
    ** Set the rcb to NULL if not used for new request.
    **
    ** Return directly in the switch for any other result
    ** (but make sure RCB is used or freed).
    */
    switch( (pkt_type = read_hdr( rcb, &hdr )) )
    {
    case DAM_TL_CR :
    case DAM_TL_CC :
    case DAM_TL_DC :
	/*
	** The Connection Request packet is expected only
	** initially and should be passed to dmtl_accept().
	**
	** The Connection Confirm packet is never expected.
	**
	** We only send a Disconnect Request when aborting
	** the connection and we don't wait for a response,
	** so we don't expect a Disconnect Confirmation.
	*/
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD unexpected DMTL packet: 0x%x\n",
		       ccb->id, pkt_type );
	break;

    case DAM_TL_DT : /* Data */
	if ( ccb->tl.msg_service )  
	{
	    if ( GCD_global.gcd_trace_level >= 3 )
		TRdisplay( "%4d    GCD DMTL received data\n", ccb->id );

	    /*
	    ** Pass to MSG Layer and exit successfully.
	    */
	    (*ccb->tl.msg_service->data)( rcb );
	    return;
	}
	break;

    case DAM_TL_INT: /* Interrupt */
	if ( ccb->tl.msg_service )
	{
	    if ( GCD_global.gcd_trace_level >= 3 )
		TRdisplay( "%4d    GCD DMTL interrupt\n", ccb->id );

	    /*
	    ** Notify MSG Layer and exit successfully.
	    ** Need to free RCB as we are expected to
	    ** consume the RCB.
	    */
	    gcd_del_rcb( rcb );
	    (*ccb->tl.msg_service->intr)( ccb );
	    return;
	}
	break;

    case DAM_TL_DR : /* Disconnect */
	if ( ccb->tl.gcc_service )
	{
	    if ( GCD_global.gcd_trace_level >= 3 )
		TRdisplay( "%4d    GCD DMTL disconnecting\n", ccb->id );

	    gcd_init_rcb( rcb );
	    hdr.msg_id = DAM_TL_DC;
	    build_hdr( rcb, &hdr );
	    (*ccb->tl.gcc_service->send)( rcb );
	    rcb = NULL;		/* Passed to GCC */
	    status = OK;	/* Shutdown, don't abort */
	}
	break;

    default : 
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD invalid DMTL packet: ID=0x%x Type=0x%x\n",
		       ccb->id, hdr.tl_id, hdr.msg_id );
	break;
    }

    dmtl_shutdown( ccb, rcb, status );
    return;
}


/*
** Name: dmtl_abort
**
** Description:
**	Called to indicate that the GCC Service has 
**	aborted the connection.  No further requests
**	(in either direction) are allowed.
**
** Input:
**	GCD_CCB *	Connection control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	15-Jan-03 (gordy)
**	    Created.
*/

static void 
dmtl_abort( GCD_CCB *ccb )
{
    /*
    ** No further requests are allowed to GCC.
    */
    ccb->tl.gcc_service = NULL;
    dmtl_shutdown( ccb, NULL, OK );
    return;
}


/*
** Name: dmtl_rcv_xoff
**
** Description:
**	Control data flow being received on a connection.
**
** Input:
**	GCD_CCB *	Connection control block
**	bool		TRUE to pause receiving data, FALSE to resume
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	15-Jan-03 (gordy)
**	    Created.
*/

static void 
dmtl_rcv_xoff( GCD_CCB *ccb, bool xoff )
{
    if ( ccb->tl.gcc_service )  (*ccb->tl.gcc_service->xoff)( ccb, xoff );
    return;
}


/*
** Name: dmtl_send
**
** Description:
**	Send data on a connection.
**
** Input:
**	GCD_RCB *	Request control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	15-Jan-03 (gordy)
**	    Created.
**	 5-Dec-07 (gordy)
**	    Packet ID now stored in CCB.
*/

static void 
dmtl_send( GCD_RCB *rcb )
{
    DAM_TL_HDR	hdr;
    GCD_CCB	*ccb = rcb->ccb;

    /*
    ** Consume RCB if we can't pass to GCC,
    */
    if ( ! ccb->tl.gcc_service )
	gcd_del_rcb( rcb );
    else
    {
	if ( GCD_global.gcd_trace_level >= 3 )
	    TRdisplay( "%4d    GCD DMTL sending data\n", ccb->id );

	/*
	** Build TL Data packet header and send.
	*/
	hdr.tl_id = ccb->tl.packet_id;
	hdr.msg_id = DAM_TL_DT;
	build_hdr( rcb, &hdr );
	(*ccb->tl.gcc_service->send)( rcb );
    }

    return;
}


/*
** Name: dmtl_disc
**
** Description:
**	Disconnect a connection.  An error code other
**	than OK indicates an abort condition.  No 
**	further requests (in either direction) are 
**	allowed.
**
** Input:
**	GCD_CCB *	Connection control block.
**	STATUS		OK or Error code.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	15-Jan-03 (gordy)
**	    Created.
*/

static void 
dmtl_disc( GCD_CCB *ccb, STATUS status )
{
    /*
    ** No further requests are allowed to the MSG layer.
    */
    ccb->tl.msg_service = NULL;

    /*
    ** The client initiates shutdown during normal operations.
    ** We only initiate shutdown for abort conditions.
    */
    if ( status != OK )  dmtl_shutdown( ccb, NULL, status );
    return;
}


/*
** Name: read_cr_parms
**
** Description:
**	Read and remove TL Connection Request parameters from RCB.
**
** Input:
**	rcb		Request control block.
**
** Output:
**	size		Data buffer size.
**	msg_parms	Message Layer parameter buffer.
**	msg_len		Length of ML parameter buffer.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	15-Jan-03 (gordy)
**	    Created.
*/

static STATUS
read_cr_parms( GCD_RCB *rcb, u_i1 *size, u_i1 **msg_parms, u_i2 *msg_len )
{
    GCD_CCB		*ccb = rcb->ccb;
    DAM_TL_PARAM	param;
    STATUS		status;

    while( rcb->buf_len >= (CV_N_I1_SZ * 2) )
    {
	CV2L_I1_MACRO( rcb->buf_ptr, &param.param_id, &status );
	rcb->buf_ptr += CV_N_I1_SZ;
	rcb->buf_len -= CV_N_I1_SZ;

	CV2L_I1_MACRO( rcb->buf_ptr, &param.pl1, &status );
	rcb->buf_ptr += CV_N_I1_SZ;
	rcb->buf_len -= CV_N_I1_SZ;

	if ( param.pl1 < 255 )
	    param.pl2 = param.pl1;
	else  if ( rcb->buf_len >= CV_N_I2_SZ )
	{
	    CV2L_I2_MACRO( rcb->buf_ptr, &param.pl2, &status );
	    rcb->buf_ptr += CV_N_I2_SZ;
	    rcb->buf_len -= CV_N_I2_SZ;
	}
	else
	{
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD no CR param length: need %d, have %d\n",
			   ccb->id, CV_N_I2_SZ, rcb->buf_len );
			
	    return( E_GC480A_PROTOCOL_ERROR );
	}

	if ( param.pl2 > rcb->buf_len )
	{
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD no CR parameter: need %d, have %d\n",
			   ccb->id, param.pl2, rcb->buf_len );
	    return( E_GC480A_PROTOCOL_ERROR );
	}

	switch( param.param_id )
	{
	case DAM_TL_CP_PROTO :
	    if ( param.pl2 == CV_N_I1_SZ )
		CV2L_I1_MACRO( rcb->buf_ptr, &ccb->tl.proto_lvl, &status );
	    else
	    {
		if ( GCD_global.gcd_trace_level >= 1 )
		    TRdisplay( "%4d    GCD CR PROTO param length: %d\n",
			       ccb->id, param.pl2 );
		return( E_GC480A_PROTOCOL_ERROR );
	    }
	    break;

	case DAM_TL_CP_SIZE :
	    if ( param.pl2 == CV_N_I1_SZ )
		CV2L_I1_MACRO( rcb->buf_ptr, size, &status );
	    else
	    {
		if ( GCD_global.gcd_trace_level >= 1 )
		    TRdisplay( "%4d    GCD CR SIZE param length: %d\n",
			       ccb->id, param.pl2 );
		return( E_GC480A_PROTOCOL_ERROR );
	    }
	    break;

	case DAM_TL_CP_MSG_ID :
	    if ( param.pl2 != CV_N_I4_SZ )
	    {
		if ( GCD_global.gcd_trace_level >= 1 )
		    TRdisplay( "%4d    GCD CR MSG ID param length: %d\n",
			       ccb->id, param.pl2 );
		return( E_GC480A_PROTOCOL_ERROR );
	    }
	    else
	    {
		u_i4 tl_id;
		CV2L_I4_MACRO( rcb->buf_ptr, &tl_id, &status );

		if ( tl_id != DAM_TL_DMML_ID )
		{
		    if ( GCD_global.gcd_trace_level >= 1 )
			TRdisplay( "%4d    GCD CR invalid MSG ID : 0x%x\n",
				   ccb->id, tl_id );
		    return( E_GC480A_PROTOCOL_ERROR );
		}
	    }
	    break;

	case DAM_TL_CP_MSG :
	    *msg_parms = rcb->buf_ptr;
	    *msg_len = param.pl2;
	    break;

	default :
	    /*
	    ** We don't service this protocol above level 1,
	    ** so only trace this warning when appropriate.
	    */
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD unknown DMTL CR param ID: %d\n",
			   ccb->id, param.param_id );
	    break;
	}

	rcb->buf_ptr += param.pl2;
	rcb->buf_len -= param.pl2;
    }

    if ( rcb->buf_len > 0 )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD extra data with CR request: %d\n",
		       ccb->id, rcb->buf_len );
	return( E_GC480A_PROTOCOL_ERROR );
    }

    return( OK );
}


/*
** Name: build_cc_parms
**
** Description:
**	Build the TL Connection Confirm packet parameters.
**
** Input:
**	rcb		Request control block.
**	size		Data buffer size.
**	msg_param	Message Layer parameter block.
**	msg_len		Length of ML parameter block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	15-Jan-03 (gordy)
**	    Created.
**	16-Jan-03 (gordy)
**	    Send the MSG layer ID along with the MSG layer info.
*/ 

static void
build_cc_parms( GCD_RCB *rcb, u_i1 size, u_i1 *msg_param, u_i2 msg_len )
{
    GCD_CCB		*ccb = rcb->ccb;
    DAM_TL_PARAM	param;
    STATUS		status;

    /*
    ** TL Protocol Level
    */
    param.param_id = DAM_TL_CP_PROTO;
    CV2N_I1_MACRO( &param.param_id, &rcb->buf_ptr[ rcb->buf_len ], &status );
    rcb->buf_len += CV_N_I1_SZ;

    param.pl1 = 1;
    CV2N_I1_MACRO( &param.pl1, &rcb->buf_ptr[ rcb->buf_len ], &status );
    rcb->buf_len += CV_N_I1_SZ;

    CV2N_I1_MACRO( &ccb->tl.proto_lvl, &rcb->buf_ptr[rcb->buf_len], &status );
    rcb->buf_len += CV_N_I1_SZ;

    /*
    ** Packet Buffer Size
    */
    param.param_id = DAM_TL_CP_SIZE;
    CV2N_I1_MACRO( &param.param_id, &rcb->buf_ptr[ rcb->buf_len ], &status );
    rcb->buf_len += CV_N_I1_SZ;

    param.pl1 = 1;
    CV2N_I1_MACRO( &param.pl1, &rcb->buf_ptr[ rcb->buf_len ], &status );
    rcb->buf_len += CV_N_I1_SZ;

    CV2N_I1_MACRO( &size, &rcb->buf_ptr[ rcb->buf_len ], &status );
    rcb->buf_len += CV_N_I1_SZ;

    /*
    ** Character Set
    */
    if ( GCD_global.charset_id  &&  ccb->tl.proto_lvl >= DAM_TL_PROTO_2 )
    {
	param.param_id = DAM_TL_CP_CHRSET_ID;
	CV2N_I1_MACRO( &param.param_id, &rcb->buf_ptr[rcb->buf_len], &status );
	rcb->buf_len += CV_N_I1_SZ;

	param.pl1 = 4;
	CV2N_I1_MACRO( &param.pl1, &rcb->buf_ptr[ rcb->buf_len ], &status );
	rcb->buf_len += CV_N_I1_SZ;

	CV2N_I4_MACRO( &GCD_global.charset_id,
			&rcb->buf_ptr[ rcb->buf_len ], &status );
	rcb->buf_len += CV_N_I4_SZ;
    }

    if ( GCD_global.charset  &&  *GCD_global.charset )
    {
	param.param_id = DAM_TL_CP_CHRSET;
	CV2N_I1_MACRO( &param.param_id, &rcb->buf_ptr[rcb->buf_len], &status );
	rcb->buf_len += CV_N_I1_SZ;

	param.pl1 = STlength( GCD_global.charset );
	CV2N_I1_MACRO( &param.pl1, &rcb->buf_ptr[ rcb->buf_len ], &status );
	rcb->buf_len += CV_N_I1_SZ;

	MEcopy( (PTR)GCD_global.charset, param.pl1,
		(PTR)&rcb->buf_ptr[ rcb->buf_len ] );
	rcb->buf_len += param.pl1;
    }

    /*
    ** Message Layer Info
    */
    if ( msg_len > 0  &&  msg_param )
    {
	/*
	** Send our MSG layer ID to identify the
	** parameters info to follow.
	*/
	if ( ccb->tl.proto_lvl >= DAM_TL_PROTO_2 )
	{
	    u_i4 ml_id = DAM_TL_DMML_ID;

	    param.param_id = DAM_TL_CP_MSG_ID;
	    CV2N_I1_MACRO(&param.param_id,&rcb->buf_ptr[rcb->buf_len],&status);
	    rcb->buf_len += CV_N_I1_SZ;

	    param.pl1 = 4;
	    CV2N_I1_MACRO( &param.pl1, &rcb->buf_ptr[ rcb->buf_len ], &status );
	    rcb->buf_len += CV_N_I1_SZ;

	    CV2N_I4_MACRO( &ml_id, &rcb->buf_ptr[ rcb->buf_len ], &status );
	    rcb->buf_len += CV_N_I4_SZ;
	}

	param.param_id = DAM_TL_CP_MSG;
	CV2N_I1_MACRO( &param.param_id, &rcb->buf_ptr[rcb->buf_len], &status );
	rcb->buf_len += CV_N_I1_SZ;

	if ( msg_len < 255 )
	{
	    param.pl1 = msg_len;
	    CV2N_I1_MACRO( &param.pl1, &rcb->buf_ptr[rcb->buf_len], &status );
	    rcb->buf_len += CV_N_I1_SZ;
	}
	else
	{
	    param.pl1 = 255;
	    CV2N_I1_MACRO( &param.pl1, &rcb->buf_ptr[rcb->buf_len], &status );
	    rcb->buf_len += CV_N_I1_SZ;

	    param.pl2 = msg_len;
	    CV2N_I2_MACRO( &param.pl1, &rcb->buf_ptr[rcb->buf_len], &status );
	    rcb->buf_len += CV_N_I2_SZ;
	}

	MEcopy( (PTR)msg_param, msg_len, (PTR)&rcb->buf_ptr[ rcb->buf_len ] );
	rcb->buf_len += msg_len;
    }

    return;
}


/*
** Name: dmtl_shutdown
**
** Description:
**	Terminates the MSG and GCC Services, if still
**	active.  The client connection will be aborted
**	if an error code is provided.  An RCB may be
**	provided which will be used or freed as needed.
**
** Input:
**	ccb	Connection control block.
**	rcb	Request control block (may be NULL).
**	status	Error code or OK.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	15-Jan-03 (gordy)
**	    Created.
*/

static void
dmtl_shutdown( GCD_CCB *ccb, GCD_RCB *rcb, STATUS status )
{
    /*
    ** Terminate the MSG service.
    */
    if ( ccb->tl.msg_service )
    {
	(*ccb->tl.msg_service->abort)( ccb );
	ccb->tl.msg_service = NULL;
    }

    /*
    ** Terminate the GCC service.
    */
    if ( ccb->tl.gcc_service )
    {
	if ( status != OK )
	{
	    /*
	    ** Abort the connection.
	    */
	    send_dr( ccb, rcb, status );
	    rcb = NULL;		/* Passed to GCC */
	}

	(*ccb->tl.gcc_service->disc)( ccb );
	ccb->tl.gcc_service = NULL;
    }

    /*
    ** Free RCB if not used above since we are
    ** expected to consume the RCB.
    */
    if ( rcb )  gcd_del_rcb( rcb );
    return;
}


/*
** Name: send_dr
**
** Description:
**	Send TL Disconnect Request.  An optional error
**	will be included in the message if other than OK.
**	The RCB (if provided) is consumed.
**
** Input:
**	ccb	Connection Control Block.
**	rcb	Request control block (may be NULL).
**	error	Abort error code (optional).
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	15-Jan-03 (gordy)
**	    Created.
**	 5-Dec-07 (gordy)
**	    Packet ID stored in CCB.
*/

static void
send_dr( GCD_CCB *ccb, GCD_RCB *rcb, STATUS error )
{
    DAM_TL_HDR	hdr;

    if ( ! ccb->tl.gcc_service )	/* Already shutdown? */
    {
	/*
	** Consume RCB if provided.
	*/
	if ( rcb )  gcd_del_rcb( rcb );
	return;
    }

    if ( GCD_global.gcd_trace_level >= 3 )
	TRdisplay( "%4d    GCD DMTL aborting connection: 0x%x\n", 
		    ccb->id, error );

    /*
    ** If RCB provided, initialize since it may 
    ** have contained data.  Allocate otherwise.
    ** Allow room for error parameter.
    */
    if ( rcb )
	gcd_init_rcb( rcb );
    else  if ( ! (rcb = gcd_new_rcb( ccb, (CV_N_I1_SZ * 2) + CV_N_I4_SZ )) )  
	return;

    /*
    ** Return error code to client when provided.
    */
    if ( error != OK  &&  error != FAIL )
    {
	DAM_TL_PARAM	param;
	STATUS		status;

	param.param_id = DAM_TL_DP_ERR;
	CV2N_I1_MACRO( &param.param_id,
		       &rcb->buf_ptr[ rcb->buf_len ], &status );
	rcb->buf_len += CV_N_I1_SZ;
	    
	param.pl1 = CV_N_I4_SZ;
	CV2N_I1_MACRO( &param.pl1,
		       &rcb->buf_ptr[ rcb->buf_len ], &status );
	rcb->buf_len += CV_N_I1_SZ;
		
	CV2N_I4_MACRO( &error, 
		       &rcb->buf_ptr[ rcb->buf_len ], &status );
	rcb->buf_len += CV_N_I4_SZ;
    }

    /*
    ** Add TL packet header and send packet.
    */
    hdr.tl_id = ccb->tl.packet_id;
    hdr.msg_id = DAM_TL_DR;
    build_hdr( rcb, &hdr );
    (*ccb->tl.gcc_service->send)( rcb );

    return;
}


/*
** Name: read_hdr
**
** Description:
**	Read and remove packet header.  The packet type is
**	returned if header is valid.
**
** Input:
**	rcb	Request control block.
**
** Output:
**	hdr	Packet header.
**
** Returns:
**	u_i2	Packet type or 0.
**
** History:
**	15-Jan-03 (gordy)
**	    Created.
*/

static u_i2
read_hdr( GCD_RCB *rcb, DAM_TL_HDR *hdr )
{
    STATUS	status;
    u_i2	type = 0;

    hdr->tl_id = 0;
    hdr->msg_id = 0;

    if ( rcb->buf_len >= DAM_TL_HDR_SZ )
    {
	CV2L_I4_MACRO( rcb->buf_ptr, &hdr->tl_id, &status );
	rcb->buf_ptr += CV_N_I4_SZ ;
	rcb->buf_len -= CV_N_I4_SZ ;

	CV2L_I2_MACRO( rcb->buf_ptr, &hdr->msg_id, &status );
	rcb->buf_ptr += CV_N_I2_SZ ;
	rcb->buf_len -= CV_N_I2_SZ ;

	/*
	** If the packet header ID is valid,
	** return the packet type.
	*/
	if ( hdr->tl_id == DAM_TL_ID_2 )  type = hdr->msg_id;
    }

    return( type );
}


/*
** Name: build_hdr
**
** Description:
**	Builds the TL packet header in the request buffer.
**	Buffer must allow space prior to current data for
**	the TL packet header.
**
** Input;
**	rcb	Request control block.
**	hdr	Packet header.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	15-Jan-03 (gordy)
**	    Created.
*/

static void
build_hdr( GCD_RCB *rcb, DAM_TL_HDR *hdr )
{
    STATUS	status;

    /*
    ** Caller is required to provide room prior to
    ** data for TL header, so reserve that space.
    */
    rcb->buf_ptr -= DAM_TL_HDR_SZ;
    rcb->buf_len += DAM_TL_HDR_SZ;

    /*
    ** Now build the header in space reserved.
    */
    CV2N_I4_MACRO( &hdr->tl_id, rcb->buf_ptr, &status );
    CV2N_I2_MACRO( &hdr->msg_id, &rcb->buf_ptr[ CV_N_I4_SZ ], &status );

    return;
}



