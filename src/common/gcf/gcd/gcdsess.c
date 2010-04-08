/*
** Copyright (c) 2004, 2007 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <er.h>
#include <me.h>
#include <mh.h>
#include <qu.h>
#include <st.h>
#include <tr.h>

#include <iicommon.h>
#include <gcu.h>
#include <gcdint.h>
#include <gcdapi.h>
#include <gcdmsg.h>

/*
** Name: gcdsess.c
**
** Description:
**	GCD client session handling and MSG Service Providers
**	for JDBC and DAM-ML.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	10-Sep-99 (gordy)
**	    Parameterized initial session info.
**	14-Sep-99 (gordy)
**	    Implemented JDBC error messages.
**	 4-Nov-99 (gordy)
**	    Added jdbc_sess_interrupt() to interrupt a session.
**	16-Nov-99 (gordy)
**	    Skip unknown session parameters.
**	13-Dec-99 (gordy)
**	    Calculate the data space available for messages.
**	17-Jan-00 (gordy)
**	    Added connection pooling and select-loop processing.
**	 1-Mar-00 (gordy)
**	    Transaction info moved to CIB.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-Oct-00 (gordy)
**	    Callbacks now maintained on stack.  Reworked
**	    jdbc_del_stmt() to use stack for nested request.
**	    Added jdbc_purge_stmt().  Allocate RCB in
**	    jdbc_sess_error() if needed (CCB added to params).
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
**	18-Dec-02 (gordy)
**	    Converted for Data Access Server (GCD).
**	20-Dec-02 (gordy)
**	    Message header ID now protocol level dependent.
**	15-Jan-03 (gordy)
**	    Moved statement functions to gcdstmt.c and message
**	    processing to gcdmsg.c.  Abstracted interface by 
**	    implementing Server Provider interface between 
**	    Messaging and Transport layers.
**	16-Jun-04 (gordy)
**	    Added Session Mask parameter.
**	21-Apr-06 (gordy)
**	    Added gcd_find_sess();
**	29-Jun-06 (lunbr01)  Bug 115970
**	    Remove call to gcd_sess_term() in abort_session() when 
**	    connection already being closed because it should only be
**	    called once -- at point where ccb->cib->conn is set to NULL.
**	 5-Dec-07 (gordy)
**	    Moved "CLOSED" check to more appropriate place in GCC interface.
*/	

/*
** Forward references
*/

static	STATUS	dam_init( TL_MSG_SRVC *, GCD_CCB *, u_i1 **, u_i2 * );
static	STATUS	jdbc_init( TL_MSG_SRVC *, GCD_CCB *, u_i1 **, u_i2 * );
static	STATUS	sess_parms( GCD_CCB *, u_i1, u_i1 **, u_i2 * );
static	void	sess_intr( GCD_CCB * );
static	void	sess_abort( GCD_CCB * );
static	void	abort_session( GCD_CCB * );
static	void	abort_cmpl( PTR arg );


/*
** MSG Service Providers for DAM-ML and JDBC messaging.
*/

GLOBALDEF MSG_SRVC	gcd_dam_service =
{
    dam_init,
    gcd_msg_xoff,
    gcd_msg_process,
    sess_intr,
    sess_abort,
};

GLOBALDEF MSG_SRVC	gcd_jdbc_service =
{
    jdbc_init,
    gcd_msg_xoff,
    gcd_msg_process,
    sess_intr,
    sess_abort,
};

GLOBALREF GCULIST msgMap[];


/*
** Name: dam_init
**
** Description:
**	Begin a new DAM session.
**
** Input:
**	tl_service	TL Service Provider.
**	ccb		Connection control block.
**	params		Connection parameters.
**	length		Length of parameters.
**
** Output:
**	params		Negotiated parameters.
**	length		Length of parameters.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	15-Jan-03 (gordy)
**	    Created.
**	01-Sept-04 (wansh01)
**	    Checked "CLOSED" flag and return proper error. 	
**	 5-Dec-07 (gordy)
**	    Moved "CLOSED" check to more appropriate place in GCC interface.
*/

static STATUS
dam_init( TL_MSG_SRVC *tl_srvc, GCD_CCB *ccb, u_i1 **params, u_i2 *length )
{
    STATUS status;

    if ( (status = sess_parms( ccb, MSG_DFLT_PROTO, params, length )) == OK )  
    {
	/*
	** We are accepting the connection.  Since we didn't allocate
	** the CCB, we bump the use count so it can't be freed until
	** we release it.  Also save the TL Service Provider and set
	** the DAM-ML header ID.
	*/
	ccb->use_cnt++;
	ccb->msg.tl_service = tl_srvc;
	ccb->msg.msg_hdr_id = MSG_HDR_ID_2;

	if ( GCD_global.gcd_trace_level >= 2 )
	{
	    TRdisplay( "%4d    GCD New DA-MSG connection\n", ccb->id );

	    if ( GCD_global.gcd_trace_level >= 3 )
	    {
		TRdisplay( "%4d        protocol level: %d\n", 
			   ccb->id, ccb->msg.proto_lvl );

		if ( ccb->msg.dtmc )
		    TRdisplay( "%4d        DTMC\n", ccb->id );
	    }
	}
    }

    return( status );
}


/*
** Name: jdbc_init
**
** Description:
**	Begin a new JDBC session.
**
** Input:
**	tl_service	TL Service Provider.
**	ccb		Connection control block.
**	params		Connection parameters.
**	length		Length of parameters.
**
** Output:
**	params		Negotiated parameters.
**	length		Length of parameters.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	15-Jan-03 (gordy)
**	    Created.
**	01-Sept-04 (wansh01)
**	    Checked "CLOSED" flag and return proper error. 	
**	 5-Dec-07 (gordy)
**	    Moved "CLOSED" check to more appropriate place in GCC interface.
*/

static STATUS
jdbc_init( TL_MSG_SRVC *tl_srvc, GCD_CCB *ccb, u_i1 **params, u_i2 *length )
{
    STATUS status;

    if ( (status = sess_parms( ccb, MSG_PROTO_2, params, length )) == OK )  
    {
	/*
	** We are accepting the connection.  Since we didn't allocate
	** the CCB, we bump the use count so it can't be freed until
	** we release it.  Also save the TL Service Provider and set
	** the JDBC message header ID.
	*/
	ccb->use_cnt++;
	ccb->msg.tl_service = tl_srvc;
	ccb->msg.msg_hdr_id = MSG_HDR_ID_1;

	if ( GCD_global.gcd_trace_level >= 2 )
	{
	    TRdisplay( "%4d    GCD New JDBC connection\n", ccb->id );

	    if ( GCD_global.gcd_trace_level >= 3 )
	    {
		TRdisplay( "%4d        protocol level: %d\n", 
			   ccb->id, ccb->msg.proto_lvl );

		if ( ccb->msg.dtmc )
		    TRdisplay( "%4d        DTMC\n", ccb->id );
	    }
	}
    }

    return( status );
}


/*
** Name: sess_parms
**
** Description:
**	Validates and negotiates connection parameters.
**
** Input:
**	ccb		Connection control block.
**	max_proto	Maximum protocol level.
**	params		Connection parameters.
**	length		Length of parameters.
**
** Output:
**	params		Negotiated parameters.
**	length		Length of parameters.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	10-Sep-99 (gordy)
**	    Parameterized initial session info.
**	16-Nov-99 (gordy)
**	    Skip unknown session parameters to allow for
**	    future extensibility (client can't know our
**	    protocol level until our response is received.
**	13-Dec-99 (gordy)
**	    Calculate the data space available for messages.
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
**	20-Dec-02 (gordy)
**	    Message header ID now protocol level dependent.
**	15-Jan-03 (gordy)
**	    Abstracted for Service Provider interface.
**	16-Jun-04 (gordy)
**	    Added Session Mask parameter.
*/

static STATUS
sess_parms( GCD_CCB *ccb, u_i1 max_proto, u_i1 **params, u_i2 *length )
{
    u_i1	*pbuff = *params;
    u_i2	plen = *length;
    u_i1	param_id, pl1;
    u_i2	pl2;
    STATUS	status;

    *length = 0;

    while( plen >= (CV_N_I1_SZ * 2) )
    {
	CV2L_I1_MACRO( pbuff, &param_id, &status );
	pbuff += CV_N_I1_SZ;
	plen -= CV_N_I1_SZ;

	CV2L_I1_MACRO( pbuff, &pl1, &status );
	pbuff += CV_N_I1_SZ;
	plen -= CV_N_I1_SZ;

	if ( pl1 < 255 )
	    pl2 = pl1;
	else  if ( plen >= CV_N_I2_SZ )
	{
	    CV2L_I2_MACRO( pbuff, &pl2, &status );
	    pbuff += CV_N_I2_SZ;
	    plen -= CV_N_I2_SZ;
	}
	else
	{
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD no extended length indicator (%d)\n",
			    ccb->id, plen );
	    return( E_GC480A_PROTOCOL_ERROR );
	}

	if ( pl2 > plen )  
	{
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD parameter too long (%d,%d)\n",
			    ccb->id, pl2, plen );
	    return( E_GC480A_PROTOCOL_ERROR );
	}

	switch( param_id )
	{
	case MSG_P_PROTO :
	    if ( pl2 == CV_N_I1_SZ )
		CV2L_I1_MACRO( pbuff, &ccb->msg.proto_lvl, &status );
	    else
	    {
		if ( GCD_global.gcd_trace_level >= 1 )
		    TRdisplay( "%4d    GCD invalid PROTO length %d\n",
				ccb->id, pl2 );
		return( E_GC480A_PROTOCOL_ERROR );
	    }
	    break;

	case MSG_P_DTMC :
	    if ( pl2 != 0 )
	    {
		if ( GCD_global.gcd_trace_level >= 1 )
		    TRdisplay( "%4d    GCD invalid DTMC length %d\n",
				ccb->id, pl2 );
		return( E_GC480A_PROTOCOL_ERROR );
	    }

	    ccb->msg.dtmc = TRUE;
	    break;

	case MSG_P_MASK :
	    /*
	    ** Currently, only an eight-byte session mask is
	    ** supported.  Anything less is ignored (no mask 
	    ** will be returned to client).  Anything greater
	    ** is truncated.  For any future change in protocol,
	    ** the client will be able to tell from our response
	    ** how to handle the mask.
	    */
	    if ( pl2 >= GCD_SESS_MASK_MAX )
	    {
		i4 index;

		for( index = 0; index < GCD_SESS_MASK_MAX; )
		{
		    CV2L_I1_MACRO( &pbuff[ index ], 
				   &ccb->msg.mask[ index ], &status );
		    index += CV_N_I1_SZ;
		}

		ccb->msg.mask_len = GCD_SESS_MASK_MAX;
	    }
	    break;

	default :
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD unknown session parameter ID: %d\n",
			    ccb->id, param_id );
	    break;
	}

	pbuff += pl2;
	plen -= pl2;
    }

    if ( plen > 0 )  
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD extraneous parameter data (%d bytes)\n",
			ccb->id, plen );
	return( E_GC480A_PROTOCOL_ERROR );
    }

    if ( ccb->msg.proto_lvl < MSG_PROTO_1 )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD Invalid protocol level %d\n",
			ccb->id, ccb->msg.proto_lvl );
	return( E_GC480A_PROTOCOL_ERROR );
    }
    
    /*
    ** Negotiate connection parameters.
    */
    if ( ccb->msg.proto_lvl > max_proto )
	ccb->msg.proto_lvl = max_proto;
    
    /*
    ** Return parameters to client
    */
    pbuff = ccb->msg.params;
    plen = 0;
    param_id = MSG_P_PROTO;
    CV2N_I1_MACRO( &param_id, &pbuff[ plen ], &status );
    plen += CV_N_I1_SZ;

    pl1 = 1;
    CV2N_I1_MACRO( &pl1, &pbuff[ plen ], &status );
    plen += CV_N_I1_SZ;

    CV2N_I1_MACRO( &ccb->msg.proto_lvl, &pbuff[ plen ], &status );
    plen += CV_N_I1_SZ;

    if ( ccb->msg.dtmc )
    {
	param_id = MSG_P_DTMC;
	CV2N_I1_MACRO( &param_id, &pbuff[ plen ], &status );
	plen += CV_N_I1_SZ;

	pl1 = 0;
	CV2N_I1_MACRO( &pl1, &pbuff[ plen ], &status );
	plen += CV_N_I1_SZ;
    }

    if ( ccb->msg.mask_len > 0 )
    {
	i4 index;

	param_id = MSG_P_MASK;
	CV2N_I1_MACRO( &param_id, &pbuff[ plen ], &status );
	plen += CV_N_I1_SZ;

	pl1 = ccb->msg.mask_len;
	CV2N_I1_MACRO( &pl1, &pbuff[ plen ], &status );
	plen += CV_N_I1_SZ;

	/*
	** Generate server session mask and
	** combine with client session mask.
	*/
	for( index = 0; index < ccb->msg.mask_len; index++ )
	{
	    pl1 = (u_i1)(MHrand() * 256);
	    CV2N_I1_MACRO( &pl1, &pbuff[ plen ], &status );
	    plen += CV_N_I1_SZ;
	    ccb->msg.mask[ index ] ^= pl1;
	}
    }

    *params = pbuff;
    *length = plen;
    return( OK );
}


/*
** Name: sess_intr
**
** Description:
**	Interrupt the query/statement processing of a session.
**	This function is a no-op if the session is not currently
**	active.  Errors are ignored if the session is not in an
**	interruptable state.  If the interrupt succeeds, the
**	statement being processed will receive an error status.
**
** Input:
**	ccb	Connection control block.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 4-Nov-99 (gordy)
**	    Created.
**	15-Jan-03 (gordy)
**	    Abstracted for Service Provider interface.
*/

static void
sess_intr( GCD_CCB *ccb )
{
    GCD_PCB *pcb;

    if ( ! (pcb = gcd_new_pcb( ccb )) )
    {
	gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	return;
    }

    if ( GCD_global.gcd_trace_level >= 2 )
	TRdisplay( "%4d    GCD Cancelling DBMS request\n", ccb->id );

    /*
    ** No callback is pushed, so the PCB will be
    ** deleted when the cancel operation completes.
    */
    gcd_api_cancel( pcb );
    return;
}


/*
** Name: sess_abort
**
** Description:
**	End a GCD session.  Releases the connection
**	control block when the DBMS connection has
**	closed.
**
** Input:
**	ccb	Connection control block.
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
sess_abort( GCD_CCB *ccb )
{
    /*
    ** No further requests should be forwarded
    ** to the TL Service Provider once an abort
    ** has been indicated, so drop the service.
    */
    ccb->msg.tl_service = NULL;
    abort_session( ccb );
    return;
}


/*
** Name: gcd_sess_error
**
** Description:
**	Retrieves the information for an error code and sends
**	a GCD Error message to the client.  RCB is allocated
**	if needed.
**
** Input:
**	ccb		Connection control block.
**	rcb_ptr		Request control block.
**	error		Error code.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	14-Sep-99 (gordy)
**	    Created.
**	11-Oct-00 (gordy)
**	    Allocate RCB if not passed in.  CCB added to
**	    parameters to permit RCB allocation.
**	21-Apr-06 (gordy)
**	    Don't need to pre-allocate RCB.
*/

void
gcd_sess_error( GCD_CCB *ccb, GCD_RCB **rcb_ptr, STATUS error )
{
    i4		length;
    char	msg[ ER_MAX_LEN ];
    char	sqlState[ DB_SQLSTATE_STRING_LEN ];
    STATUS	status;
    CL_ERR_DESC	cl_err;

    status = ERslookup( error, NULL, ER_TEXTONLY, sqlState, msg, sizeof( msg ),
			GCD_global.language, &length, &cl_err, 0, NULL );

    if ( status != OK )
    {
	CL_ERR_DESC	junk;
	i4		len;

	STprintf( msg, "Couldn't look up message %x (ER error %x)\n",
		  error, status );
	length = STlength( msg );

	status = ERslookup( status, &cl_err, 0, NULL, 
			    &msg[ length ], sizeof( msg ) - length,
			    GCD_global.language, &len, &junk, 0, NULL );
	if ( status == OK )  length += len;
    }

    gcd_send_error( ccb, rcb_ptr, 
    		    MSG_ET_ERR, error, sqlState, (u_i2)length, msg );
    return;
}


/*
** Name: gcd_sess_term
**
** Description:
**	Perform final cleanup of a DBMS session.
**
** Input:
**	ccb	Connection control block.
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

void
gcd_sess_term( GCD_CCB *ccb )
{
    /*
    ** Notify the TL Service Provider that the connection
    ** may be disconnected (if not previously done).  No
    ** further TL requests are allowed.
    */
    if ( ccb->msg.tl_service )  
    {
	(*ccb->msg.tl_service->disc)( ccb, OK );
	ccb->msg.tl_service = NULL;
    }

    /*
    ** Free our claim to the CCB (as established via the
    ** usage counter in dam_init() and jdbc_init()).
    */
    gcd_del_ccb( ccb );
    return;
}


/*
** Name: gcd_sess_abort
**
** Description:
**	Abort session.  Shutdown both client and DBMS connections.
**
** Input:
**	ccb	Connection control block.
**	status	Error code for abort.
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

void
gcd_sess_abort( GCD_CCB *ccb, STATUS status )
{
    if ( status != OK  &&  status != FAIL )
    {
	gcu_erlog( 0, GCD_global.language, E_GC4807_CONN_ABORT, NULL, 0, NULL );
	gcu_erlog( 0, GCD_global.language, status, NULL, 0, NULL );
    }

    if ( ccb->msg.tl_service )  
    {
	(*ccb->msg.tl_service->disc)( ccb, status );
	ccb->msg.tl_service = NULL;
    }

    abort_session( ccb );
    return;
}


/*
** Name: abort_session
**
** Description:
**	Abort DBMS connection (if still connected).
**
** Input:
**	ccb	Connection control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	17-Jan-00 (gordy)
**	    Connection handle moved to info block for pooling.
**	 1-Mar-00 (gordy)
**	    Transaction info moved to CIB.
**	11-Oct-00 (gordy)
**	    Callbacks now maintained on stack.
**	15-Jan-03 (gordy)
**	    Abstracted for Service Provider interface.
**	29-Jun-06 (lunbr01)  Bug 115970
**	    Remove call to gcd_sess_term() when connection already being
**	    closed because it should only be called once -- at point where 
**	    ccb->cib->conn is set to NULL.
*/

static void
abort_session( GCD_CCB *ccb )
{
    GCD_PCB	*pcb;

    if ( ! ccb->cib )
    {
	/*
	** DBMS session not yet set up.
	*/
	if ( GCD_global.gcd_trace_level >= 2 )
	    TRdisplay( "%4d    GCD DBMS connection not set up\n", ccb->id );
	gcd_sess_term( ccb );

    }
    else if ( ! ccb->cib->conn )
    {
	/*
	** DBMS session is closed or in the process of being closed.
	*/
	if ( GCD_global.gcd_trace_level >= 2 )
	    TRdisplay( "%4d    GCD DBMS connection closed\n", ccb->id );
    }
    else  if ( ! (pcb = gcd_new_pcb( ccb )) )
    {
	/*
	** There is no way to drop the connection
	** to the server.  Clean-up the connection
	** to the client the best we can.
	*/
	ccb->cib->conn = ccb->cib->tran = NULL;
	gcd_sess_term( ccb );
    }
    else
    {
	if ( GCD_global.gcd_trace_level >= 2 )
	    TRdisplay( "%4d    GCD aborting DBMS connection\n", ccb->id );

	pcb->data.disc.conn = ccb->cib->conn;
	ccb->cib->conn = ccb->cib->tran = NULL;
	gcd_push_callback( pcb, abort_cmpl );
	gcd_api_abort( pcb );
    }

    return;
}

static void
abort_cmpl( PTR arg )
{
    GCD_PCB	*pcb = (GCD_PCB *)arg;
    GCD_CCB	*ccb = pcb->ccb;

    gcd_del_pcb( pcb );
    gcd_sess_term( ccb );
    return;
}

/*
** Name: gcd_sess_find
**
** Description:
**	Given an API connection handle, return the associated
**	GCD connection control block.
**
** Input:
**	conn		API connection handle.
**
** Output:
**	None.
**
** Returns:
**	GCD_CCB *	GCD Connection Control Block.
**
** History:
**	21-Apr-06 (gordy)
**	    Created.
*/

GCD_CCB *
gcd_sess_find( PTR conn )
{
    QUEUE *q;

    for( q = GCD_global.ccb_q.q_next; q != &GCD_global.ccb_q; q = q->q_next )
    {
	GCD_CCB *ccb = (GCD_CCB *)q;
	if ( ccb->cib  &&  ccb->cib->conn == conn )  return( ccb );
    }

    return( NULL );
}

