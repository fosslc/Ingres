/*
** Copyright (c) 2004 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <me.h>
#include <st.h>
#include <tr.h>

#include <iicommon.h>
#include <gcu.h>
#include <jdbc.h>
#include <jdbcapi.h>
#include <jdbcmsg.h>

/*
** Name: jdbcmsg.c
**
** Description:
**	JDBC server message processing.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	14-Sep-99 (gordy)
**	    Implemented JDBC error messages.
**	29-Sep-99 (gordy)
**	    Implemented support for BLOB data.  Extracted message
**	    specific functions to their own files.
**	18-Oct-99 (gordy)
**	    Added flow control to GCC/MSG sub-systems.
**	 4-Nov-99 (gordy)
**	    Changed calling sequence for jdbc_send_done() so that
**	    ownership of RCB can be passed to communications layer.
**	13-Dec-99 (gordy)
**	    Send fetch limit result parameter in DONE message.
**	17-Jan-00 (gordy)
**	    Added a new message type for pre-defined server requests.
**	19-May-00 (gordy)
**	    Send end-of-data result parameter.
**	25-Aug-00 (gordy)
**	    Send procedure return value result parameter in DONE msg.
**	15-Nov-00 (gordy)
**	    Send cursor read-only result parameter.
**	20-Jan-01 (gordy)
**	    Added messaging protocol level 2.
*/	

GLOBALDEF GCULIST msgMap[] =
{
    { JDBC_MSG_CONNECT,	"CONNECT" },
    { JDBC_MSG_DISCONN,	"DISCONNECT" },
    { JDBC_MSG_XACT,	"XACT" },
    { JDBC_MSG_QUERY,	"QUERY" },
    { JDBC_MSG_CURSOR,	"CURSOR" },
    { JDBC_MSG_DESC,	"DESC" },
    { JDBC_MSG_DATA,	"DATA" },
    { JDBC_MSG_ERROR,	"ERROR" },
    { JDBC_MSG_DONE,	"DONE" },
    { JDBC_MSG_REQUEST, "REQUEST" },
    { 0, NULL }
};

static struct
{
    u_i1	current_state;

#define	MSG_S_IDLE	0
#define	MSG_S_READY	1
#define	MSG_S_PROC	2

#define	MSG_S_CNT	3

    u_i1	message_id;
    u_i1	next_state;

} msg_state_table[] =
{
    { MSG_S_IDLE,  JDBC_MSG_CONNECT,	MSG_S_READY },
    { MSG_S_READY, JDBC_MSG_DISCONN,	MSG_S_IDLE },
    { MSG_S_READY, JDBC_MSG_XACT,	MSG_S_PROC },
    { MSG_S_READY, JDBC_MSG_QUERY,	MSG_S_PROC },
    { MSG_S_READY, JDBC_MSG_CURSOR,	MSG_S_PROC },
    { MSG_S_READY, JDBC_MSG_REQUEST,	MSG_S_PROC },
    { MSG_S_PROC,  JDBC_MSG_DESC,	MSG_S_PROC },
    { MSG_S_PROC,  JDBC_MSG_DATA,	MSG_S_PROC },
};

static u_i1 msg_states[ MSG_S_CNT ][ JDBC_MSG_CNT ];

static GCULIST	stateMap[] =
{
    { MSG_S_IDLE,	"IDLE" },
    { MSG_S_READY,	"READY" },
    { MSG_S_PROC,	"PROCESS" },
    { 0, NULL }
};

static	void	process_messages( JDBC_CCB * );
static	bool	scan_msg_list( JDBC_CCB * );
static	bool	chk_msg_state( JDBC_CCB * );


/*
** Name: jdbc_msg_notify
**
** Description:
**	Notifies the messaging sub-system that a new message
**	has been received on a connection.  If idle, the
**	messaging sub-system will be activated to process the
**	new message.  If active, the messaging system will
**	process the new message when the current processing
**	is complete.
**
** Input:
**	ccb	Connection control block.
**
** Output:
**	None.
**
** Returns:
**	bool	TRUE if idle system activated, FALSE if already active.
**
** History:
**	18-Oct-99 (gordy)
**	    Created.
*/

bool
jdbc_msg_notify( JDBC_CCB *ccb )
{
    bool idle = (! (ccb->msg.flags & JDBC_MSG_ACTIVE));
    if ( idle )  process_messages( ccb );
    return( idle );
}


/*
** Name: jdbc_msg_pause
**
** Description:
**	Pauses the messaging sub-system waiting for additional
**	messages.  If messages have been queued, the messaging
**	system will be immediatly re-activated.  Otherwise,
**	the system will be activated by the next call to
**	jdbc_msg_notify().  If the current request has completed,
**	the message processing state will be reset to READY.
**
** Input:
**	ccb		Connection control block.
**	complete	Current request has completed.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	18-Oct-99 (gordy)
**	    Created.
*/

void
jdbc_msg_pause( JDBC_CCB *ccb, bool complete )
{
    ccb->msg.flags &= ~JDBC_MSG_ACTIVE;

    if ( complete  &&  ccb->msg.state == MSG_S_PROC )
    {
	if ( JDBC_global.trace_level >= 4 )
	    TRdisplay( "%4d    JDBC message complete: state %s -> %s\n", 
			ccb->id, gcu_lookup( stateMap, MSG_S_PROC ),
			gcu_lookup( stateMap, MSG_S_READY ) );
	ccb->msg.state = MSG_S_READY;
    }

    process_messages( ccb );
    return;
}


/*
** Name: jdbc_msg_xoff
**
** Description:
**
** Input:
**	ccb	Connection control block.
**	pause	TRUE to disable outbound IO.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	18-Oct-99 (gordy)
**	    Created.
*/

void
jdbc_msg_xoff( JDBC_CCB *ccb, bool pause )
{
    if ( pause )
    {
	if ( ! (ccb->msg.flags & JDBC_MSG_XOFF)  &&
	     JDBC_global.trace_level >= 5 )
	    TRdisplay( "%4d    JDBC OB XOFF requested\n", ccb->id );

	ccb->msg.flags |= JDBC_MSG_XOFF;
    }
    else  if ( ! ccb->msg.xoff_pcb )
    {
	if ( ccb->msg.flags & JDBC_MSG_XOFF  &&  JDBC_global.trace_level >= 5 )
	    TRdisplay( "%4d    JDBC OB XON requested\n", ccb->id );

	ccb->msg.flags &= ~JDBC_MSG_XOFF;
    }
    else
    {
	JDBC_PCB *pcb = (JDBC_PCB *)ccb->msg.xoff_pcb;

	if ( JDBC_global.trace_level >= 4 )
	    TRdisplay( "%4d    JDBC OB XON requested: posting fetch\n",
		       ccb->id );

	ccb->msg.flags &= ~JDBC_MSG_XOFF;
	ccb->msg.xoff_pcb = NULL;
	jdbc_api_getCol( pcb );
    }

    return;
}


/*
** Name: process_messages
**
** Description:
**	Process a JDBC message.
**
** Input:
**	rcb		Request control block.
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
**	18-Oct-99 (gordy)
**	    Added flow control to GCC/MSG sub-systems.  Made
**	    function local.  Added handling of flushed messages.
**	    Reworked EOD/EOG checking.
**	17-Jan-00 (gordy)
**	    Added a new message type for pre-defined server requests.
*/

static void
process_messages( JDBC_CCB *ccb )
{
    /*
    ** If message flushing has been started, get next 
    ** message and flush it.  Loop terminates when EOG 
    ** message is flushed (jdbc_flush_msg() clears 
    ** JDBC_MSG_FLUSH) or no more messages are queued.
    */
    while( ccb->msg.flags & JDBC_MSG_FLUSH  &&  scan_msg_list( ccb ) )
	jdbc_flush_msg( ccb );

    if ( ccb->msg.flags & JDBC_MSG_FLUSH  ||  ! scan_msg_list( ccb ) )
    {
	/*
	** No message to process, so enable inbound client IO.
	*/
	jdbc_gcc_xoff( ccb, FALSE );
    }
    else  if ( chk_msg_state( ccb ) )
    {
	switch( ccb->msg.msg_id )
	{
	case JDBC_MSG_QUERY :		/* Messages don't require EOG */
	case JDBC_MSG_DESC :
	case JDBC_MSG_DATA :
	case JDBC_MSG_REQUEST :
	    break;

	case JDBC_MSG_CONNECT :		/* Messages require EOG */
	case JDBC_MSG_DISCONN :
	case JDBC_MSG_XACT :
	case JDBC_MSG_CURSOR :
	    if ( ! (ccb->msg.msg_flags & JDBC_MSGFLG_EOG) )
	    {
		if ( JDBC_global.trace_level >= 1 )
		    TRdisplay( "%4d    JDBC message requires EOG\n", ccb->id );
		jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
		return;
	    }
	    break;

	default :
	    if ( JDBC_global.trace_level >= 1 )
		TRdisplay( "%4d    JDBC invalid message ID: %d\n", 
			    ccb->id, ccb->msg.msg_id );
	    jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
	    return;
	}

	/*
	** Dispatch message for processing.
	*/
	ccb->msg.flags |= JDBC_MSG_ACTIVE;

	switch( ccb->msg.msg_id )
	{
	case JDBC_MSG_CONNECT :	jdbc_msg_connect( ccb );	break;
	case JDBC_MSG_DISCONN :	jdbc_msg_disconnect( ccb );	break;
	case JDBC_MSG_XACT :	jdbc_msg_xact( ccb );		break;
	case JDBC_MSG_QUERY :	jdbc_msg_query( ccb );		break;
	case JDBC_MSG_CURSOR :	jdbc_msg_cursor( ccb );		break;
	case JDBC_MSG_DESC :	jdbc_msg_desc( ccb );		break;
	case JDBC_MSG_DATA :	jdbc_msg_data( ccb );		break;
	case JDBC_MSG_REQUEST : jdbc_msg_request( ccb );	break;
	}
    }

    return;
}


/*
** Name: scan_msg_list
**
** Description:
**	Scan the connection message list and determine
**	overall message info.
**
** Input:
**	ccb	    Connection control block.
**
** Output:
**	ccb
**	 msg
**	  msg_id    Message type.
**	  msg_len   Length of message.
**	  msg_flags Message flags.
**
** Returns:
**	bool	    TRUE if successful, FALSE otherwise.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	18-Oct-99 (gordy)
**	    Added flow control to GCC/MSG sub-systems.  Reworked
**	    EOD/EOG checking.
*/

static bool
scan_msg_list( JDBC_CCB *ccb )
{
    JDBC_RCB	*rcb, *next = NULL;
    bool	success = FALSE;

    ccb->msg.msg_len = 0;
    ccb->msg.msg_flags = 0;

    for(
	 rcb = (JDBC_RCB *)ccb->msg.msg_q.q_next;
	 (QUEUE *)rcb != &ccb->msg.msg_q;
	 rcb = next
       )
    {
	if ( ! next )
	    ccb->msg.msg_id = rcb->msg.msg_id;
	else  if ( ccb->msg.msg_id != rcb->msg.msg_id )
	{
	    if ( JDBC_global.trace_level >= 1 )
		TRdisplay( "%4d    JDBC invalid message continuation\n", 
			    ccb->id );
	    jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
	    success = FALSE;
	    break;
	}

	/*
	** Accumulate the message length and flags.
	** Drop any empty RCB (which should only
	** happen for an empty message).  Check 
	** for the end of the message.
	*/
	ccb->msg.msg_len += rcb->msg.msg_len;
	ccb->msg.msg_flags |= rcb->msg.msg_flags;
	next = (JDBC_RCB *)rcb->q.q_next;

	if ( ! rcb->msg.msg_len )
	{
	    QUremove( &rcb->q );
	    jdbc_del_rcb( rcb );
	}

	if ( ccb->msg.msg_flags & JDBC_MSGFLG_EOD )
	{
	    if ( JDBC_global.trace_level >= 4 )
		TRdisplay( "%4d    JDBC process message %s\n", 
			    ccb->id, gcu_lookup( msgMap, ccb->msg.msg_id ) );
	    success = TRUE;

	    /*
	    ** If this is the last RCB on the queue,
	    ** enable inbound IO to continue filling
	    ** the queue.
	    */
	    if ( (QUEUE *)next == &ccb->msg.msg_q )
		jdbc_gcc_xoff( ccb, FALSE );
	    break;
	}
    }

    return( success );
}


/*
** Name: chk_msg_state
**
** Description:
**	Validates the current message processing state for a
**	new message.  Determines the new processing state and
**	returns and indication if the message should be processed.
**
** Input:
**	ccb	Connection control block.
**
** Output:
**	None.
**
** Returns:
**	bool	TRUE if message should be processed, FALSE otherwise.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	18-Oct-99 (gordy)
**	    Simplified message processing.  Moved message flushing
**	    to process_message().
*/

static bool
chk_msg_state( JDBC_CCB *ccb )
{
    bool success;

    if ( ! msg_states[ 0 ][ 0 ] )	/* Initialize state tables */
    {
	u_i4	i;

	MEfill( sizeof( msg_states ), 0xff, (PTR)msg_states );

	for( i = 0; i < ARR_SIZE( msg_state_table ); i++ )
	    msg_states[ msg_state_table[i].current_state ]
		      [ msg_state_table[i].message_id ] = i;
    }

    if ( ccb->msg.state >= MSG_S_CNT  ||  ccb->msg.msg_id >= JDBC_MSG_CNT  ||
         msg_states[ ccb->msg.state ][ ccb->msg.msg_id ] == 0xff )
    {
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC invalid message state (%d %d)\n", 
			ccb->id, ccb->msg.state, ccb->msg.msg_id );
	jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
	success = FALSE;
    }
    else
    {
	u_i1 index = msg_states[ ccb->msg.state ][ ccb->msg.msg_id ];
	u_i1 next_state = msg_state_table[ index ].next_state;

	if ( JDBC_global.trace_level >= 4 )
	    TRdisplay( "%4d    JDBC message state %s -> %s\n", 
			ccb->id, gcu_lookup( stateMap, ccb->msg.state ),
			gcu_lookup( stateMap, next_state ) );

	ccb->msg.state = next_state;
	success = TRUE;
    }

    return( success );
}


/*
** Name: jdbc_flush_msg
**
** Description:
**	Flush current message.  If current message is not EOG,
**	sets the flushing flag so that subsequent messages will
**	be flushed.
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
**	 3-Jun-99 (gordy)
**	    Created.
**	18-Oct-99 (gordy)
**	    Renamed for external reference.  Only flushes the
**	    current message.
*/

void
jdbc_flush_msg( JDBC_CCB *ccb )
{
    if ( JDBC_global.trace_level >= 4 )
	TRdisplay( "%4d    JDBC flushing message %s\n", 
		    ccb->id, gcu_lookup( msgMap, ccb->msg.msg_id ) );

    /*
    ** Set/clear flush flag if current message isn't/is EOG.
    */
    if ( ccb->msg.msg_flags & JDBC_MSGFLG_EOG )
    	ccb->msg.flags &= ~JDBC_MSG_FLUSH;
    else
	ccb->msg.flags |= JDBC_MSG_FLUSH;

    /*
    ** Delete request control blocks associated with message.
    */
    while( ccb->msg.msg_len )
    {
	JDBC_RCB	*rcb;
	
	rcb = (JDBC_RCB *)ccb->msg.msg_q.q_next;

	if ( (QUEUE *)rcb == &ccb->msg.msg_q  || 
	     rcb->msg.msg_id != ccb->msg.msg_id )
	{
	    /* Should not happen! */
	    ccb->msg.msg_len = 0;
	    break;
	}

	if ( rcb->msg.msg_len > ccb->msg.msg_len )
	    ccb->msg.msg_len = 0;	/* should not happen! */
	else
	    ccb->msg.msg_len -= rcb->msg.msg_len;

	QUremove( &rcb->q );
	jdbc_del_rcb( rcb );
    }

    return;
}


/*
** Name: jdbc_send_done
**
** Description:
**	Send a JDBC_MSG_DONE message to the client.  The
**	message is appended to an existing request control
**	block if provided (in which case the request control
**	block must not be used after this call completes). 
**	Otherwise a new control block is allocated.  Optional 
**	result parameters may also be provided.
**
** Input:
**	ccb	Connection control block.
**	rcb_ptr	Request control block, may be NULL.
**	pcb	Parameter control block, may be NULL.
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
**	18-Oct-99 (gordy)
**	    Renamed for external reference.  No longer 
**	    recurses for message processing.
**	 4-Nov-99 (gordy)
**	    Added one level of indirection on the RCB so that
**	    ownership is definitively passed to the comm layer.
**	13-Dec-99 (gordy)
**	    Send fetch limit result parameter.
**	19-May-00 (gordy)
**	    Send end-of-data result parameter.
**	25-Aug-00 (gordy)
**	    Send procedure return value result parameter.
**	15-Nov-00 (gordy)
**	    Send cursor read-only result parameter.
**	20-Jan-01 (gordy)
**	    EOD, PROC_RESULT, READ_ONLY sent at protocol level 2 and up.
*/

void
jdbc_send_done( JDBC_CCB *ccb, JDBC_RCB **rcb_ptr, JDBC_PCB *pcb )
{
    JDBC_RCB *rcb = NULL;

    if ( rcb_ptr )
    {
	rcb = *rcb_ptr;
	*rcb_ptr = NULL;
    }

    if ( ! rcb  &&  ! (rcb = jdbc_new_rcb( ccb )) )
    {
	jdbc_gcc_abort( ccb, E_JD0108_NO_MEMORY );
	return;
    }

    jdbc_msg_begin( &rcb, JDBC_MSG_DONE, 0 );

    if ( pcb )
    {
	if ( pcb->result.flags & PCB_RSLT_ROWCOUNT )
	{
	    jdbc_put_i2( &rcb, JDBC_RP_ROWCOUNT );
	    jdbc_put_i2( &rcb, CV_N_I4_SZ );
	    jdbc_put_i4( &rcb, pcb->result.rowcount );
	}

	if ( pcb->result.flags & PCB_RSLT_STMT_ID )
	{
	    jdbc_put_i2( &rcb, JDBC_RP_STMT_ID );
	    jdbc_put_i2( &rcb, CV_N_I4_SZ * 2 );
	    jdbc_put_i4( &rcb, pcb->result.stmt_id.id_high );
	    jdbc_put_i4( &rcb, pcb->result.stmt_id.id_low );
	}

	if ( pcb->result.flags & PCB_RSLT_FETCH_LIMIT )
	{
	    jdbc_put_i2( &rcb, JDBC_RP_FETCH_LIMIT );
	    jdbc_put_i2( &rcb, CV_N_I4_SZ );
	    jdbc_put_i4( &rcb, pcb->result.fetch_limit );
	}

	if ( pcb->result.flags & PCB_RSLT_PROC_RET  &&
	     ccb->conn_info.proto_lvl >= JDBC_MSG_PROTO_2 )
	{
	    jdbc_put_i2( &rcb, JDBC_RP_PROC_RESULT );
	    jdbc_put_i2( &rcb, CV_N_I4_SZ );
	    jdbc_put_i4( &rcb, pcb->result.proc_ret );
	}

	if ( pcb->result.flags & PCB_RSLT_XACT_END )
	{
	    jdbc_put_i2( &rcb, JDBC_RP_XACT_END );
	    jdbc_put_i2( &rcb, 0 );
	}

	if ( pcb->result.flags & PCB_RSLT_EOD  &&
	     ccb->conn_info.proto_lvl >= JDBC_MSG_PROTO_2 )
	{
	    jdbc_put_i2( &rcb, JDBC_RP_EOD );
	    jdbc_put_i2( &rcb, 0 );
	}

	if ( pcb->result.flags & PCB_RSLT_READ_ONLY  &&
	     ccb->conn_info.proto_lvl >= JDBC_MSG_PROTO_2 )
	{
	    jdbc_put_i2( &rcb, JDBC_RP_READ_ONLY );
	    jdbc_put_i2( &rcb, 0 );
	}
    }

    jdbc_msg_end( rcb, TRUE );
    return;
}


/*
** Name: jdbc_send_error
**
** Description:
**	Format and send an ERROR message.
**
** Input:
**	rcb_ptr		Request control block.
**	type		JDBC error type.
**	error		Error code.
**	sqlState	SQL State.
**	len		Length of text.
**	msg		Message text.
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
*/

void
jdbc_send_error( JDBC_RCB **rcb_ptr, u_i1 type, STATUS error,
		 char *sqlState, u_i2 len, char *msg )
{
    jdbc_msg_begin( rcb_ptr, JDBC_MSG_ERROR, 
		    (u_i2)(CV_N_I4_SZ + DB_SQLSTATE_STRING_LEN + 
			   CV_N_I1_SZ + CV_N_I2_SZ + len) );
    jdbc_put_i4( rcb_ptr, error );
    jdbc_put_bytes( rcb_ptr, DB_SQLSTATE_STRING_LEN, (u_i1 *)sqlState );
    jdbc_put_i1( rcb_ptr, type );
    jdbc_put_i2( rcb_ptr, len );
    jdbc_put_bytes(rcb_ptr, len, (u_i1 *)msg );
    jdbc_msg_end( *rcb_ptr, FALSE );

    return;
}


/*
** Name: jdbc_msg_begin
**
** Description:
**	Begin a new JDBC message.  If a message had already
**	been started and not completed (with jdbc_msg_end()),
**	the current message is completed prior to starting the
**	new message.  If the new message (including requested
**	reserved space) will not fit in the buffer with any 
**	previous message, the current buffer is sent and a new 
**	request control block is allocated.
**
** Input:
**	rcb_ptr	    Request control block.
**	msg_id	    New message ID.
**	size	    Space to reserve for messae.
**
** Output:
**	rcb_ptr	    New request control block.
**
** Returns:
**	bool	    TRUE if successful, FALSE memory allocation error.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
*/

bool
jdbc_msg_begin( JDBC_RCB **rcb_ptr, u_i1 msg_id, u_i2 size )
{
    JDBC_RCB	    *rcb = *rcb_ptr;
    JDBC_CCB	    *ccb = rcb->ccb;
    JDBC_MSG_HDR    hdr;
    STATUS	    status;

    hdr.hdr_id = JDBC_MSG_HDR_ID;
    hdr.msg_id = msg_id;
    hdr.msg_len = 0;
    hdr.msg_flags = 0;

    if ( rcb->msg.msg_id )  
    {
	/*
	** Complete the previous message.  The new message
	** will be built in the same request control block.
	*/
	jdbc_msg_end( rcb, FALSE );
    }

    if ( (JDBC_MSG_HDR_LEN + size) > (rcb->buf_max - rcb->buf_len) )
    {
	jdbc_gcc_send( rcb );
	*rcb_ptr = NULL;
	if ( ! (rcb = jdbc_new_rcb( ccb )) )  return( FALSE );
	*rcb_ptr = rcb;
    }

    rcb->msg.msg_id = hdr.msg_id;
    rcb->msg.msg_hdr = rcb->buf_len;

    CV2N_I4_MACRO( &hdr.hdr_id, &rcb->buffer[ rcb->buf_len ], &status );
    rcb->buf_len += CV_N_I4_SZ;

    /*
    ** While we know the requested size, we wait
    ** until the message is complete to place the
    ** the actual length of the message in the header.
    */
    CV2N_I2_MACRO( &hdr.msg_len, &rcb->buffer[ rcb->buf_len ], &status );
    rcb->buf_len += CV_N_I2_SZ;

    CV2N_I1_MACRO( &hdr.msg_id, &rcb->buffer[ rcb->buf_len ], &status );
    rcb->buf_len += CV_N_I1_SZ;

    CV2N_I1_MACRO( &hdr.msg_flags, &rcb->buffer[ rcb->buf_len ], &status );
    rcb->buf_len += CV_N_I1_SZ;

    return( TRUE );
}


/*
** Name: jdbc_msg_end
**
** Description:
**	Complete and optionally send a message started with
**	jdbc_msg_begin().  The request control block may not
**	be used after this call if the flush parameter is
**	TRUE.
**
** Input:
**	rcb	Request control block.
**	flush	TRUE if message is to be sent, FALSE otherwise.
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
*/

void
jdbc_msg_end( JDBC_RCB *rcb, bool flush )
{
    JDBC_MSG_HDR    hdr;
    STATUS	    status;
	
    if ( ! rcb->msg.msg_id )  return;	/* No current message */
    hdr.msg_len = rcb->buf_len - rcb->msg.msg_hdr - JDBC_MSG_HDR_LEN;
    hdr.msg_flags = JDBC_MSGFLG_EOD | (flush ? JDBC_MSGFLG_EOG : 0);

    /*
    ** Update message header.
    */
    CV2N_I2_MACRO( &hdr.msg_len, &rcb->buffer[ rcb->msg.msg_hdr + 
					       CV_N_I4_SZ ], &status );
    CV2N_I1_MACRO( &hdr.msg_flags, &rcb->buffer[ rcb->msg.msg_hdr + 
		   CV_N_I4_SZ + CV_N_I2_SZ + CV_N_I1_SZ ], &status );

    if ( JDBC_global.trace_level >= 2 )
	TRdisplay( "%4d    JDBC sending message %s length %d %s %s\n", 
		   rcb->ccb->id, gcu_lookup( msgMap, rcb->msg.msg_id ), 
		   hdr.msg_len, hdr.msg_flags & JDBC_MSGFLG_EOD ? "EOD" : "",
		   hdr.msg_flags & JDBC_MSGFLG_EOG ? "EOG" : "" );

    rcb->msg.msg_id = 0;
    if ( flush )  jdbc_gcc_send( rcb );

    return;
}


/*
** Name: jdbc_msg_split
**
** Description:
**	Split the current message by completing and sending 
**	the current message segment, allocating a new request
**	control block, and building a continued message header.
**
** Input:
**	rcb_ptr	    Request control block.
**
** Output:
**	rcb_ptr	    New request control block.
**
** Returns:
**	bool	    TRUE if successful, FALSE if allocation error.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
*/

bool
jdbc_msg_split( JDBC_RCB **rcb_ptr )
{
    JDBC_RCB	    *rcb = *rcb_ptr;
    JDBC_CCB	    *ccb = rcb->ccb;
    JDBC_MSG_HDR    hdr;
    STATUS	    status;

    if ( ! rcb->msg.msg_id )  return( FALSE );	/* No current message */

    hdr.msg_id = rcb->msg.msg_id;
    hdr.msg_len = rcb->buf_len - rcb->msg.msg_hdr - JDBC_MSG_HDR_LEN;

    /*
    ** Update message header.
    */
    CV2N_I2_MACRO( &hdr.msg_len, &rcb->buffer[ rcb->msg.msg_hdr + 
					       CV_N_I4_SZ ], &status );

    if ( JDBC_global.trace_level >= 2 )
	TRdisplay( "%4d    JDBC splitting message %s length %d\n", 
		   rcb->ccb->id, gcu_lookup( msgMap, rcb->msg.msg_id ), 
		   hdr.msg_len );

    jdbc_gcc_send( rcb );
    *rcb_ptr = NULL;
    if ( ! (rcb = jdbc_new_rcb( ccb )) )  return( FALSE );
    *rcb_ptr = rcb;
    
    return( jdbc_msg_begin( rcb_ptr, hdr.msg_id, 0 ) );
}

