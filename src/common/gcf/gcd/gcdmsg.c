/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <me.h>
#include <st.h>
#include <tr.h>

#include <iicommon.h>
#include <gcu.h>
#include <gcdint.h>
#include <gcdapi.h>
#include <gcdmsg.h>

/*
** Name: gcdmsg.c
**
** Description:
**	GCD server message processing.
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
**	18-Dec-02 (gordy)
**	    Converted for Data Access Server (GCD).
**	20-Dec-02 (gordy)
**	    Message header ID now protocol level dependent.
**	15-Jan-03 (gordy)
**	    Move message list building to this file and renamed
**	    gcd_msg_notify() to gcd_msg_process().  Renamed 
**	    gcd_flush_msg() to gcd_msg_flush() for consistency.
**	    Implemented TL Service Provider interface.  Adjusted
**	    semantics of RCB buffer parameters.
**	24-Feb-03 (gordy)
**	    Return table and object keys.
**	18-Mar-2003 (wansh01)
**	    Move GLOBALDEF msgMap[] to gcddata.c  	
**	20-Aug-03 (gordy)
**	    Result flags consolidated into single parameter.
**	 1-Oct-03 (gordy)
**	    DONE message renamed to RESULT.  Made gcd_send_done()
**	    a cover for new routine gcd_send_result() which allows
**	    end-of-group to be controlled.
**	18-Aug-04 (wansh01)
**	    Moved #define MSG_S_xxxx msg current_state to gcdint.h.
**	21-Apr-06 (gordy)
**	    Outbound message processing now fully maintains RCB.
**	    Added outbound INFO message queue.
**	 4-Apr-07 (gordy)
**	    Added support for scrollable cursors.
**	11-Jun-07 (bonro01)
**	    Fix Solaris compiler error caused by previous change.
**	20-Nov-07 (rajus01) Bug 119505, SD Issue: 122906.
**	    Added support for maintaining API environment handle for each
**          supported client protocol level.
**	26-Oct-09 (gordy)
**	    Added protocol level 7.
**	25-Mar-10 (gordy)
**	    Added support for batch processing.  Added BATCH state
**	    which supports parameters like query processing, but
**	    also supports additional batch requests.
**	29-Mar-10 (Bruce Lunsford)
**	    Remove extra semicolon (typo) at end of msg_flags from prior
**	    change as it caused Windows build to fail.
**	13-May-10 (gordy)
**	    Processing TL packets containing multiple message segments
**	    more efficiently by delaying extraction of individual
**	    segments until ready to process.  Extraction is done by
**	    referencing the message segment rather than copying to a
**	    new buffer.
**	02-Sep-10 (thich01)
**	    Change IIAPI_VERSION_7 to VERSION_8 in MSG_PROTO_7 since DAM 
**	    protocol level is not being bumped.
*/	


GLOBALREF GCULIST msgMap[];

static struct
{
    u_i1	current_state;
    u_i1	message_id;
    u_i1	next_state;

} msg_state_table[] =
{
    { MSG_S_IDLE,  MSG_CONNECT,	MSG_S_READY },
    { MSG_S_READY, MSG_DISCONN,	MSG_S_IDLE },
    { MSG_S_READY, MSG_XACT,	MSG_S_PROC },
    { MSG_S_READY, MSG_QUERY,	MSG_S_PROC },
    { MSG_S_READY, MSG_CURSOR,	MSG_S_PROC },
    { MSG_S_READY, MSG_REQUEST,	MSG_S_PROC },
    { MSG_S_READY, MSG_BATCH,	MSG_S_BATCH },
    { MSG_S_BATCH, MSG_BATCH,	MSG_S_BATCH },
    { MSG_S_BATCH, MSG_DESC,	MSG_S_BATCH },
    { MSG_S_BATCH, MSG_DATA,	MSG_S_BATCH },
    { MSG_S_PROC,  MSG_DESC,	MSG_S_PROC },
    { MSG_S_PROC,  MSG_DATA,	MSG_S_PROC },
};

static u_i1 msg_states[ MSG_S_CNT ][ DAM_ML_MSG_CNT ];

static GCULIST	stateMap[] =
{
    { MSG_S_IDLE,	"IDLE" },
    { MSG_S_READY,	"READY" },
    { MSG_S_PROC,	"PROCESS" },
    { MSG_S_BATCH,	"BATCH" },
    { 0, NULL }
};

/*
** Forward References
*/

static	bool	read_msg_hdr( GCD_RCB * );
static	void	process_messages( GCD_CCB * );
static	bool	scan_msg_list( GCD_CCB * );
static	STATUS	split_msg_list( GCD_RCB ** );
static	bool	chk_msg_state( GCD_CCB * );
static	void	gcd_flush_info( GCD_CCB *ccb );
static	void	gcd_hdr_init( GCD_RCB *, u_i1 );
static	void	gcd_hdr_upd( GCD_RCB *, u_i1 );


/*
** Name: gcd_msg_process
**
** Description:
**	Appends new message to connection message queue.
**	Activates the messaging sub-system, if idle, to
**	process the new message.  
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
**	18-Oct-99 (gordy)
**	    Created.
**	15-Jan-03 (gordy)
**	    Moved building of message list into this function and renamed.
**	13-May-10 (gordy)
**	    Message segments are no longer separated on message queue.  
**	    RCB is loaded with info on initial (or only) message segment 
**	    and simply placed on the message queue.  Skip empty packets.
*/

void
gcd_msg_process( GCD_RCB *rcb )
{
    GCD_CCB	*ccb = rcb->ccb;
    STATUS	status;

    /*
    ** Skip empty NL/TL data segments.
    */
    if ( ! rcb->buf_len )
    {
	gcd_del_rcb( rcb );
	return;
    }

    /*
    ** The TL data segment may contain multiple message
    ** segments.  The RCB is initialized with info for
    ** the first (or only) message segment.  Message
    ** segments will be separated by scan_msg_list().
    */
    if ( ! read_msg_hdr( rcb ) )
    {
	gcd_del_rcb( rcb );
	gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	return;
    }

    /*
    ** Append RCB to end of connection message list.
    */
    QUinsert( &rcb->q, rcb->ccb->msg.msg_q.q_prev );

    /*
    ** Notify the messaging sub-system that there is a
    ** message ready to be processed or disable receives.
    */
    if ( ! (ccb->msg.flags & GCD_MSG_ACTIVE) )  
	process_messages( ccb );
    else  if ( ccb->msg.tl_service )  
	(*ccb->msg.tl_service->xoff)( ccb, TRUE );

    return;
}


/*
** Name: gcd_msg_pause
**
** Description:
**	Pauses the messaging sub-system waiting for additional
**	messages.  If messages have been queued, the messaging
**	system will be immediatly re-activated.  Otherwise,
**	the system will be activated by the next call to
**	gcd_msg_process().  If the current request has completed,
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
**	25-Mar-10 (gordy)
**	    Added batch processing state with same handling as
**	    query processing state..
*/

void
gcd_msg_pause( GCD_CCB *ccb, bool complete )
{
    ccb->msg.flags &= ~GCD_MSG_ACTIVE;

    switch( ccb->msg.state )
    {
    case MSG_S_PROC :
    case MSG_S_BATCH :
	if ( complete )
	{
	    if ( GCD_global.gcd_trace_level >= 4 )
		TRdisplay( "%4d    GCD message complete: state %s -> %s\n", 
			    ccb->id, gcu_lookup( stateMap, ccb->msg.state ),
			    gcu_lookup( stateMap, MSG_S_READY ) );
	    ccb->msg.state = MSG_S_READY;
	}
	break;
    }

    process_messages( ccb );
    return;
}


/*
** Name: gcd_msg_flush
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
gcd_msg_flush( GCD_CCB *ccb )
{
    if ( GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d    GCD flushing message %s\n", 
		    ccb->id, gcu_lookup( msgMap, ccb->msg.msg_id ) );

    /*
    ** Set/clear flush flag if current message isn't/is EOG.
    */
    if ( ccb->msg.msg_flags & MSG_HDR_EOG )
    	ccb->msg.flags &= ~GCD_MSG_FLUSH;
    else
	ccb->msg.flags |= GCD_MSG_FLUSH;

    /*
    ** Delete request control blocks associated with message.
    */
    while( ccb->msg.msg_len )
    {
	GCD_RCB	*rcb;
	
	rcb = (GCD_RCB *)ccb->msg.msg_q.q_next;

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
	gcd_del_rcb( rcb );
    }

    return;
}


/*
** Name: read_msg_hdr
**
** Description:
**	Read a message header.  Validate message structure.
**
** Input:
**	rcb	    Request control block.
**
** Output:
**	rcb
**	 msg
**	  msg_id    Message type.
**	  msg_len   Length of message segment.
**	  msg_flags Header flags: EOD, EOG.
**
** Returns:
**	bool	    TRUE if header read.  FALSE if malformed header.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	20-Dec-02 (gordy)
**	    Message header ID now protocol level dependent.
**	15-Jan-03 (gordy)
**	    Moved to this file.  Adjusted semantics of RCB buffer parameters.
**	13-May-10 (gordy)
**	    Check the buffer length against the length extracted from
**	    the header.  The prior test against the un-updated RCB length
**	    results in failure when processing multiple message segments
**	    in a single TL packet.  This bug had not previously been hit
**	    because clients always packed a single message segment in
**	    a TL packet even though multiple segments should have been
**	    supported.
*/

static bool
read_msg_hdr( GCD_RCB *rcb )
{
    GCD_CCB	*ccb = rcb->ccb;
    DAM_ML_HDR	hdr;
    STATUS	status;

    /*
    ** Headers may not be split.
    */
    if ( rcb->buf_len < MSG_HDR_LEN )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD insufficient data for message header\n", 
		       ccb->id );
	return( FALSE );
    }

    CV2L_I4_MACRO( rcb->buf_ptr, &hdr.hdr_id, &status );
    rcb->buf_ptr += CV_N_I4_SZ;
    rcb->buf_len -= CV_N_I4_SZ;

    CV2L_I2_MACRO( rcb->buf_ptr, &hdr.msg_len, &status );
    rcb->buf_ptr += CV_N_I2_SZ;
    rcb->buf_len -= CV_N_I2_SZ;

    CV2L_I1_MACRO( rcb->buf_ptr, &hdr.msg_id, &status );
    rcb->buf_ptr += CV_N_I1_SZ;
    rcb->buf_len -= CV_N_I1_SZ;

    CV2L_I1_MACRO( rcb->buf_ptr, &hdr.msg_flags, &status );
    rcb->buf_ptr += CV_N_I1_SZ;
    rcb->buf_len -= CV_N_I1_SZ;

    /*
    ** Validate header info.
    */
    if ( hdr.hdr_id != ccb->msg.msg_hdr_id )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD invalid message header ID: 0x%x\n", 
		       ccb->id, hdr.hdr_id );
	return( FALSE );
    }

    /*
    ** A message segment should be contained
    ** within a NL/TL data segment.  NL/TL
    ** are required to provide an entire data
    ** segment even if split during transport.
    */
    if ( hdr.msg_len > rcb->buf_len )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD insufficient data message segment\n",
			ccb->id );
	return( FALSE );
    }

    rcb->msg.msg_id = hdr.msg_id;
    rcb->msg.msg_len = hdr.msg_len;
    rcb->msg.msg_flags = hdr.msg_flags;

    if ( GCD_global.gcd_trace_level >= 2 )
	TRdisplay( "%4d    GCD receive message %s length %d%s%s%s\n", 
		   rcb->ccb->id, gcu_lookup( msgMap, rcb->msg.msg_id ), 
		   rcb->msg.msg_len,
		   rcb->msg.msg_flags & MSG_HDR_EOD ? " EOD" : "",
		   rcb->msg.msg_flags & MSG_HDR_EOB ? " EOB" : "",
		   rcb->msg.msg_flags & MSG_HDR_EOG ? " EOG" : "" );

    return( TRUE );
}


/*
** Name: process_messages
**
** Description:
**	Process a GCD message.
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
**	15-Jan-03 (gordy)
**	    Implemented TL Service Provider interface.
**	25-Mar-25 (gordy)
**	    Added batch message.  IO must only be enabled once all
**	    processing in this routine has completed or be done in
**	    some protected block of code, otherwise, the call may 
**	    recurse resulting in corrupted processing.  This would 
**	    occur because the recursive call to scan_msg_list() 
**	    would see the same message, process it, and then the 
**	    CCB information would be incorrect when the recursion 
**	    completes. Must enable client IO if no message available
**	    because it is no longer done in scan_msg_list().
*/

static void
process_messages( GCD_CCB *ccb )
{
    bool first;

    /*
    ** If message flushing has been started, get next message 
    ** and flush it.  Loop ends when EOG message is flushed 
    ** (gcd_msg_flush() clears GCD_MSG_FLUSH flag) or no more 
    ** messages are ready for processing.
    */
    while( ccb->msg.flags & GCD_MSG_FLUSH )
    {
	if ( ! scan_msg_list( ccb ) )
	{
	    /*
	    ** No message to process, enable inbound client IO.
	    */
	    if ( ccb->msg.tl_service ) 
		(*ccb->msg.tl_service->xoff)( ccb, FALSE );
	    return;
	}

	gcd_msg_flush( ccb );
    }

    /*
    ** Check if there is a message available to be processed.
    */
    if ( ! scan_msg_list( ccb ) )
    {
	/*
	** No message to process, enable inbound client IO.
	*/
	if ( ccb->msg.tl_service )  (*ccb->msg.tl_service->xoff)( ccb, FALSE );
	return;
    }

    if ( GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d    GCD process message %s\n", 
		    ccb->id, gcu_lookup( msgMap, ccb->msg.msg_id ) );

    /*
    ** Verify and update message processing state.
    */
    first = (ccb->msg.state == MSG_S_READY);	/* First message in group */
    if ( ! chk_msg_state( ccb ) )  return;

    /*
    ** Validate message type and check EOG if required.
    */
    switch( ccb->msg.msg_id )
    {
    case MSG_QUERY :		/* Messages don't require EOG */
    case MSG_DESC :
    case MSG_DATA :
    case MSG_REQUEST :
    case MSG_BATCH :
	break;

    case MSG_CONNECT :		/* Messages require EOG */
    case MSG_DISCONN :
    case MSG_XACT :
    case MSG_CURSOR :
	if ( ! (ccb->msg.msg_flags & MSG_HDR_EOG) )
	{
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD message requires EOG\n", ccb->id );
	    gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	    return;
	}
	break;

    default :
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD invalid message ID: %d\n", 
			ccb->id, ccb->msg.msg_id );
	gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	return;
    }

    /*
    ** Dispatch message for processing.
    */
    ccb->msg.flags |= GCD_MSG_ACTIVE;

    switch( ccb->msg.msg_id )
    {
    case MSG_CONNECT : gcd_msg_connect( ccb );		break;
    case MSG_DISCONN : gcd_msg_disconnect( ccb );	break;
    case MSG_XACT    : gcd_msg_xact( ccb );		break;
    case MSG_QUERY   : gcd_msg_query( ccb );		break;
    case MSG_CURSOR  : gcd_msg_cursor( ccb );		break;
    case MSG_DESC    : gcd_msg_desc( ccb );		break;
    case MSG_DATA    : gcd_msg_data( ccb );		break;
    case MSG_REQUEST : gcd_msg_request( ccb );		break;
    case MSG_BATCH   : gcd_msg_batch( ccb, first );	break;
    }

    /*
    ** If the message queue is now empty, enable inbound client IO.
    */
    if ( ccb->msg.msg_q.q_next == &ccb->msg.msg_q  &&  ccb->msg.tl_service )
	(*ccb->msg.tl_service->xoff)( ccb, FALSE );
    return;
}


/*
** Name: scan_msg_list
**
** Description:
**	Scan the connection message list and determine overall message info.
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
**	15-Jan-03 (gordy)
**	    Implemented TL Service Provider interface.
**	25-Mar-10 (gordy)
**	    Don't enable IO, leave that as an action for the caller
**	    to determine.  Don't update CCB until a full message is 
**	    available.
**	29-Mar-10 (Bruce Lunsford)
**	    Remove extra semicolon (typo) at end of msg_flags from prior
**	    change as it caused Windows build to fail.
**	13-May-10 (gordy)
**	    TL packets with multiple message segments are no longer
**	    separated when first received.  The RCB has been loaded
**	    with the message info for the first (or only) message
**	    segment.  If there are multiple message segments in the
**	    RCB, separate out the first for separate processing.
*/

static bool
scan_msg_list( GCD_CCB *ccb )
{
    GCD_RCB	*rcb, *next = NULL;
    u_i1	msg_id;
    u_i1	msg_flags = 0;
    u_i4	msg_len = 0;
    STATUS	status;
    bool	success = FALSE;

    for(
	 rcb = (GCD_RCB *)ccb->msg.msg_q.q_next;
	 (QUEUE *)rcb != &ccb->msg.msg_q;
	 rcb = next
       )
    {
	/*
	** Each RCB on the message list is initialized with
	** the message info for the message contained.  In
	** the case of an RCB containing multiple message
	** segments, the message info is for the first
	** message segment in the RCB.
	**
	** Save message type or verify proper continuation.
	*/
	if ( ! next )
	    msg_id = rcb->msg.msg_id;
	else  if ( msg_id != rcb->msg.msg_id )
	{
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD invalid message continuation\n", 
			    ccb->id );
	    gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	    break;
	}

	/*
	** If RCB contains multiple message segments, separate 
	** out the first message segment for processing.
	*/
	if ( rcb->msg.msg_len < rcb->buf_len  &&  
	     (status = split_msg_list( &rcb )) != OK )
	{
	    gcd_sess_abort( ccb, status );
	    break;
	}

	/*
	** Accumulate the message length and flags.  Drop any 
	** empty RCB (which should only happen for an empty 
	** message).  Check for the end of the message.
	*/
	msg_len += rcb->msg.msg_len;
	msg_flags |= rcb->msg.msg_flags;
	next = (GCD_RCB *)rcb->q.q_next;

	if ( ! rcb->msg.msg_len )
	{
	    QUremove( &rcb->q );
	    gcd_del_rcb( rcb );
	}

	if ( msg_flags & MSG_HDR_EOD )
	{
	    success = TRUE;
	    ccb->msg.msg_id = msg_id;
	    ccb->msg.msg_len = msg_len;
	    ccb->msg.msg_flags = msg_flags;
	    break;
	}
    }

    return( success );
}


/*
** Name: split_msg_list
**
** Description:
**	Process a request control block by separating
**	message segments into individual control blocks
**	and placing the blocks on the connection message
**	list for simplified access.
**
**	Only the first message segment in the provided
**	RCB is separated and the new RCB is returned.
**	The new RCB is also inserted into the message
**	list preceding the provided RCB.
**
**	Nothing is done if provided RCB does not contain
**	multiple message segments.
**
** Input:
**	rcb		Request control block.
**
** Output:
**	rcb		RCB of next segment.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	13-May-10 (gordy)
**	    Created (converted from build_msg_list()).
*/

static STATUS
split_msg_list( GCD_RCB **rcb_ptr )
{
    GCD_RCB	*rcb = *rcb_ptr;
    GCD_RCB	*new_rcb;

    /*
    ** Nothing to do if message segment is empty
    ** or it is the only segment in the RCB.
    */
    if ( ! rcb->msg.msg_len  ||  rcb->msg.msg_len >= rcb->buf_len )
	return( OK );

    /*
    ** Extract leading message segment into new RCB.
    ** The message segment is left in place and simply
    ** referenced by the new RCB and existing RCB is
    ** updated to reference the next segment.
    */
    if ( ! (new_rcb = gcd_new_rcb( rcb->ccb, 0 )) )
	return( E_GC4808_NO_MEMORY );

    new_rcb->buf_ptr = rcb->buf_ptr;
    new_rcb->buf_len = rcb->msg.msg_len;
    new_rcb->msg.msg_id = rcb->msg.msg_id;
    new_rcb->msg.msg_len = rcb->msg.msg_len;
    new_rcb->msg.msg_flags = rcb->msg.msg_flags;
    rcb->buf_ptr += rcb->msg.msg_len;
    rcb->buf_len -= rcb->msg.msg_len;

    /*
    ** Load next message segment info for buffer where
    ** current message segment was just removed.
    */
    if ( ! read_msg_hdr( rcb ) )  
    {
	gcd_del_rcb( new_rcb );
	return( E_GC480A_PROTOCOL_ERROR );
    }

    /*
    ** Insert new message segment in front of buffer
    ** from where it was removed and return the new
    ** RCB to the caller.
    */
    QUinsert( &new_rcb->q, rcb->q.q_prev );
    *rcb_ptr = new_rcb;

    return( OK );
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
chk_msg_state( GCD_CCB *ccb )
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

    if ( ccb->msg.state >= MSG_S_CNT  ||  ccb->msg.msg_id >= DAM_ML_MSG_CNT  ||
         msg_states[ ccb->msg.state ][ ccb->msg.msg_id ] == 0xff )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD invalid message state (%d %d)\n", 
			ccb->id, ccb->msg.state, ccb->msg.msg_id );
	gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	success = FALSE;
    }
    else
    {
	u_i1 index = msg_states[ ccb->msg.state ][ ccb->msg.msg_id ];
	u_i1 next_state = msg_state_table[ index ].next_state;

	if ( GCD_global.gcd_trace_level >= 4 )
	    TRdisplay( "%4d    GCD message state %s -> %s\n", 
			ccb->id, gcu_lookup( stateMap, ccb->msg.state ),
			gcu_lookup( stateMap, next_state ) );

	ccb->msg.state = next_state;
	success = TRUE;
    }

    return( success );
}


/*
** Name: gcd_msg_xoff
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
gcd_msg_xoff( GCD_CCB *ccb, bool pause )
{
    if ( pause )
    {
	if ( ! (ccb->msg.flags & GCD_MSG_XOFF)  &&
	     GCD_global.gcd_trace_level >= 5 )
	    TRdisplay( "%4d    GCD OB XOFF requested\n", ccb->id );

	ccb->msg.flags |= GCD_MSG_XOFF;
    }
    else  if ( ! ccb->msg.xoff_pcb )
    {
	if ( ccb->msg.flags & GCD_MSG_XOFF && GCD_global.gcd_trace_level >= 5 )
	    TRdisplay( "%4d    GCD OB XON requested\n", ccb->id );

	ccb->msg.flags &= ~GCD_MSG_XOFF;
    }
    else
    {
	GCD_PCB *pcb = (GCD_PCB *)ccb->msg.xoff_pcb;

	if ( GCD_global.gcd_trace_level >= 4 )
	    TRdisplay( "%4d    GCD OB XON requested: continue processing\n",
		       ccb->id );

	ccb->msg.flags &= ~GCD_MSG_XOFF;
	ccb->msg.xoff_pcb = NULL;
	gcd_api_getCol( pcb );
    }

    return;
}


/*
** Name: gcd_send_done
**
** Description:
**	Send final MSG_RESULT message to the client.  The
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
**	 1-Oct-03 (gordy)
**	    Extracted actual message processing to gcd_send_result().
**	    This function remains as a usefull cover providing rcb
**	    handling and forcing message to be sent.
**	21-Apr-06 (gordy)
**	    No longer need to pre-allocate RCB.  RCB pointers now
**	    fully managed by gcd_send_result().
**	25-Mar-10 (gordy)
**	    Replaced flush flag parameter to gcd_send_result() with
**	    message header flags.
*/

void
gcd_send_done( GCD_CCB *ccb, GCD_RCB **rcb_ptr, GCD_PCB *pcb )
{
    GCD_RCB	*rcb = NULL;

    /*
    ** If no RCB pointer is passed in, use local pointer.
    ** Since we flush the RESULT message, we don't need 
    ** to worry about an orphaned RCB.
    */
    if ( ! rcb_ptr )  rcb_ptr = &rcb;
    gcd_send_result( ccb, rcb_ptr, pcb, MSG_HDR_EOG );
    return;
}


/*
** Name: gcd_send_result
**
** Description:
**	Send a MSG_RESULT message to the client.  Optional 
**	result parameters may also be provided.  The message
**	is marked EOD along with any additional header flags
**	provided.  The request control block may not be used 
**	after this call if EOG header flag is set.
**
** Input:
**	ccb	Connection control block.
**	rcb_ptr	Request control block.
**	pcb	Parameter control block, may be NULL.
**	flags	Message header flags.
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
**	24-Feb-03 (gordy)
**	    Return table and object keys (protocol level 3).
**	20-Aug-03 (gordy)
**	    Result flags consolidated into single parameter.
**	 1-Oct-03 (gordy)
**	    DONE message renamed to RESULT.  Extracted from gcd_send_done()
**	    to allow non-end-of-group RESULT messages.
**	 4-Apr-07 (gordy)
**	    Return scrollable cursor flag.
**	25-Mar-10 (gordy)
**	    Replace flush flag with general message header flags.
*/

void
gcd_send_result
(
    GCD_CCB	*ccb,
    GCD_RCB	**rcb_ptr,
    GCD_PCB	*pcb,
    u_i1	flags
)
{
    gcd_msg_begin( ccb, rcb_ptr, MSG_RESULT, 0 );

    if ( pcb )
    {
	if ( pcb->result.flags & PCB_RSLT_ROW_STATUS  &&
	     ccb->msg.proto_lvl >= MSG_PROTO_6 )
	{
	    gcd_put_i2( rcb_ptr, MSG_RP_ROW_STATUS );
	    gcd_put_i2( rcb_ptr, CV_N_I2_SZ );
	    gcd_put_i2( rcb_ptr, pcb->result.row_status );

	    gcd_put_i2( rcb_ptr, MSG_RP_ROW_POS );
	    gcd_put_i2( rcb_ptr, CV_N_I4_SZ );
	    gcd_put_i4( rcb_ptr, pcb->result.row_position );
	}

	if ( pcb->result.flags & PCB_RSLT_ROW_COUNT )
	{
	    gcd_put_i2( rcb_ptr, MSG_RP_ROWCOUNT );
	    gcd_put_i2( rcb_ptr, CV_N_I4_SZ );
	    gcd_put_i4( rcb_ptr, pcb->result.row_count );
	}

	if ( pcb->result.flags & PCB_RSLT_STMT_ID )
	{
	    gcd_put_i2( rcb_ptr, MSG_RP_STMT_ID );
	    gcd_put_i2( rcb_ptr, CV_N_I4_SZ * 2 );
	    gcd_put_i4( rcb_ptr, pcb->result.stmt_id.id_high );
	    gcd_put_i4( rcb_ptr, pcb->result.stmt_id.id_low );
	}

	if ( pcb->result.flags & PCB_RSLT_FETCH_LIMIT )
	{
	    gcd_put_i2( rcb_ptr, MSG_RP_FETCH_LIMIT );
	    gcd_put_i2( rcb_ptr, CV_N_I4_SZ );
	    gcd_put_i4( rcb_ptr, pcb->result.fetch_limit );
	}

	if ( pcb->result.flags & PCB_RSLT_PROC_RET  &&
	     ccb->msg.proto_lvl >= MSG_PROTO_2 )
	{
	    gcd_put_i2( rcb_ptr, MSG_RP_PROC_RESULT );
	    gcd_put_i2( rcb_ptr, CV_N_I4_SZ );
	    gcd_put_i4( rcb_ptr, pcb->result.proc_ret );
	}

	if ( ccb->msg.proto_lvl >= MSG_PROTO_3 )
	{
	    /*
	    ** Flag values have been consolidated into a single
	    ** parameter at message protocol level 3 and above.
	    */
	    u_i1 flags = 0;

	    if ( pcb->result.flags & PCB_RSLT_TBLKEY )
	    {
		u_i2 size = sizeof( pcb->result.tblkey );
		gcd_put_i2( rcb_ptr, MSG_RP_TBLKEY );
		gcd_put_i2( rcb_ptr, size );
		gcd_put_bytes( rcb_ptr, size, pcb->result.tblkey );
	    }

	    if ( pcb->result.flags & PCB_RSLT_OBJKEY )
	    {
		u_i2 size = sizeof( pcb->result.objkey );
		gcd_put_i2( rcb_ptr, MSG_RP_OBJKEY );
		gcd_put_i2( rcb_ptr, size );
		gcd_put_bytes( rcb_ptr, size, pcb->result.objkey );
	    }

	    if ( pcb->result.flags & PCB_RSLT_XACT_END )
		flags |= MSG_RF_XACT_END;

	    if ( pcb->result.flags & PCB_RSLT_READ_ONLY )
		flags |= MSG_RF_READ_ONLY;

	    if ( pcb->result.flags & PCB_RSLT_SCROLL )
		flags |= MSG_RF_SCROLL;

	    if ( pcb->result.flags & PCB_RSLT_EOD )
		flags |= MSG_RF_EOD;

	    if ( pcb->result.flags & PCB_RSLT_CLOSED )
		flags |= MSG_RF_CLOSED;

	    if ( flags )
	    {
		gcd_put_i2( rcb_ptr, MSG_RP_FLAGS );
		gcd_put_i2( rcb_ptr, (u_i2)1 );
		gcd_put_i1( rcb_ptr, flags );
	    }
	}
	else
	{
	    /*
	    ** Send individual flag parameters when 
	    ** messaging protocol level below 3.
	    */
	    if ( pcb->result.flags & PCB_RSLT_XACT_END )
	    {
		gcd_put_i2( rcb_ptr, MSG_RP_XACT_END );
		gcd_put_i2( rcb_ptr, 0 );
	    }

	    if ( pcb->result.flags & PCB_RSLT_EOD  &&
		 ccb->msg.proto_lvl >= MSG_PROTO_2 )
	    {
		gcd_put_i2( rcb_ptr, MSG_RP_EOD );
		gcd_put_i2( rcb_ptr, 0 );
	    }

	    if ( pcb->result.flags & PCB_RSLT_READ_ONLY  &&
		 ccb->msg.proto_lvl >= MSG_PROTO_2 )
	    {
		gcd_put_i2( rcb_ptr, MSG_RP_READ_ONLY );
		gcd_put_i2( rcb_ptr, 0 );
	    }
	}

	/*
	** Clear result indicators since results sent.
	*/
	pcb->result.flags = 0;
    }

    gcd_msg_complete( rcb_ptr, flags );
    return;
}


/*
** Name: gcd_send_error
**
** Description:
**	Format and send an ERROR message.
**
** Input:
**	rcb_ptr		Request control block.
**	type		GCD error type.
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
**	21-Apr-06 (gordy)
**	    Added CCB parameter.
*/

void
gcd_send_error( GCD_CCB *ccb, GCD_RCB **rcb_ptr, 
		u_i1 type, STATUS error, char *sqlState, u_i2 len, char *msg )
{
    gcd_msg_begin( ccb, rcb_ptr, MSG_ERROR, 
		    (u_i2)(CV_N_I4_SZ + DB_SQLSTATE_STRING_LEN + 
			   CV_N_I1_SZ + CV_N_I2_SZ + len) );
    gcd_put_i4( rcb_ptr, error );
    gcd_put_bytes( rcb_ptr, DB_SQLSTATE_STRING_LEN, (u_i1 *)sqlState );
    gcd_put_i1( rcb_ptr, type );
    gcd_put_i2( rcb_ptr, len );
    gcd_put_bytes( rcb_ptr, len, (u_i1 *)msg );
    gcd_msg_end( rcb_ptr, FALSE );

    return;
}


/*
** Name: gcd_send_trace
**
** Description:
**	Send a trace message to the client.  The message
**	may be queued until an appropriate point in the
**	communications protocol is reached.
**
** Input:
**	ccb	Connection control block.
**	len	Length of message.
**	msg	Trace message.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	21-Apr-06 (gordy)
**	    Created.
*/

void
gcd_send_trace( GCD_CCB *ccb, u_i2 len, char *msg )
{
    GCD_RCB	*rcb = NULL;
    u_i2	trc_len = (CV_N_I2_SZ * 2) + len;	/* id, len, txt */
    char	*txt;

    /*
    ** INFO messages only sent at MSG protocol level 5
    ** and above.  Information is ignored otherwise.
    */
    if ( ccb->msg.proto_lvl < MSG_PROTO_5 )  return;

    /*
    ** Each RCB comprises a single INFO message which
    ** may contain multiple elements.  If there are 
    ** RCB's queued for output, check to see if trace 
    ** element can be appended.
    */
    if ( ! QUEUE_EMPTY( &ccb->msg.info_q ) )  
    {
    	rcb = (GCD_RCB *)ccb->msg.info_q.q_prev;	/* last buffer */

	/*
	** The trace message text needs an EOS terminator
	** for logging, but there isn't a buffer available
	** to hold the text for formatting.  An additional 
	** byte is reserved in the RCB to permit formatting 
	** even though it won't actually be used otherwise.
	**
	** Check to see if new trace element can be appended
	** to existing buffer.
	*/ 
	if ( (trc_len + 1) > RCB_AVAIL( rcb ) )
        {
	    /*
	    ** INFO messages are sent once an entire buffer
	    ** is filled.  An active message on the channel
	    ** will delay flushing until the active message
	    ** is complete (see gcd_msg_complete()).  In 
	    ** either case, the RCB can't be used for the 
	    ** current trace element.
	    */
	    if ( ! ccb->msg.msg_active )  gcd_flush_info( ccb );
	    rcb = NULL;
	}
    }

    /*
    ** If there isn't an RCB available to hold the
    ** current trace element, allocate a new buffer.
    */
    if ( ! rcb )
    {
        if ( ! (rcb = gcd_new_rcb( ccb, -1 )) )
	{
	    /*
	    ** Memory allocation failure at this point is not treated
	    ** as critical because INFO messages are not considered a 
	    ** critical part of client communications.  Simply log the 
	    ** failure without sending the trace message.
	    */
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD mem alloc failed for trace msg RCB\n", 
			    ccb->id );
	    return;
	}

	/*
	** The INFO message is initialized before the RCB
	** is placed on the message queue.  Subsequent info
	** elements will be appended to this message. The 
	** message will be completed when removed from the 
	** queue (see gcd_flush_info()).
	*/
	gcd_hdr_init( rcb, MSG_INFO );
    	QUinsert( &rcb->q, ccb->msg.info_q.q_prev );
    }

    /*
    ** Format INFO trace element.
    */
    gcd_put_i2( &rcb, MSG_IP_TRACE );
    gcd_put_i2( &rcb, len );

    /*
    ** We made sure there was room for the formatted
    ** trace text in the buffer.  Save text position 
    ** in buffer and terminate text.
    */
    txt = (char *)&rcb->buf_ptr[ rcb->buf_len ];
    gcd_put_bytes( &rcb, len, (u_i1 *)msg );
    txt[ len ] = EOS;

    if ( GCD_global.gcd_trace_level >= 3 )
        TRdisplay( "%4d    GCD Trace msg: %s\n", ccb->id, txt );

    return;
}


/*
** Name: gcd_flush_info
**
** Description:
**	Send all RCB's queued for output on the info queue.
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
**	21-Apr-06 (gordy)
**	    Created.
*/

static void
gcd_flush_info( GCD_CCB *ccb )
{
    GCD_RCB *rcb; 

    while( (rcb = (GCD_RCB *)QUremove( ccb->msg.info_q.q_next )) )
    {
	/*
	** INFO messages are initiated when first placed
	** on the info queue and subsequently have elements
	** appended.  Complete the message now that it is
	** ready to be sent.
	*/
	gcd_hdr_upd( rcb, MSG_HDR_EOD );

	/*
	** Send the message.  The RCB is simply freed if the
	** TL service has been terminated.
	*/
	if ( ccb->msg.tl_service )  
	    (*ccb->msg.tl_service->send)( rcb );
	else
	    gcd_del_rcb( rcb );
    }

    return;
}


/*
** Name: gcd_msg_begin
**
** Description:
**	Begin a new GCD message.  The message should be finalized
**	using gcd_msg_end() or gcd_msg_complete() and may be split 
**	using gcd_msg_split().  
**
**	A new buffer will be allocated if there isn't a current 
**	buffer (*rcb_ptr == NULL).
**
**	If the previous message is incomplete, it will be completed 
**	prior to starting the new message.  
**
**	If there is insufficient space in the current buffer for 
**	the requested reserved space, the current buffer will be 
**	sent and a new buffer allocated for the new message.  
**
**	A successful return indicates that a valid buffer exists.
**	The RCB pointer will be NULL if failure is returned.
**
** Input:
**	ccb	    Connection control block.
**	rcb_ptr	    Request control block.
**	msg_id	    New message ID.
**	size	    Space to reserve for message.
**
** Output:
**	rcb_ptr	    Request control block.
**
** Returns:
**	bool	    TRUE if successful, FALSE memory allocation error.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	20-Dec-02 (gordy)
**	    Message header ID now protocol level dependent.
**	15-Jan-03 (gordy)
**	    Implemented TL Service Provider interface.  Adjusted
**	    semantics of RCB buffer parameters.
**	21-Apr-06 (gordy)
**	    Allow input buffer to be NULL, which requires a CCB
**	    to be passed in.
*/

bool
gcd_msg_begin( GCD_CCB *ccb, GCD_RCB **rcb_ptr, u_i1 msg_id, u_i2 size )
{
    GCD_RCB *rcb = *rcb_ptr;

    if ( rcb )
    {
	/*
	** Complete previous message but re-use current buffer.
	*/
	if ( rcb->msg.msg_id )  gcd_msg_end( &rcb, FALSE );

	/*
	** Send current buffer if insufficient space available.
	**
	** The current buffer is also sent if INFO messages have
	** been queued so that the queue can be flushed prior to 
	** the new message.
	*/
	if ( (MSG_HDR_LEN + size) > RCB_AVAIL( rcb )  ||
	     ! QUEUE_EMPTY( &ccb->msg.info_q ) )
	{
	    /*
	    ** Send the message.  The RCB is simply freed if the
	    ** TL service has been terminated.
	    */
	    if ( ccb->msg.tl_service )  
		(*ccb->msg.tl_service->send)( rcb );
	    else
		gcd_del_rcb( rcb );

	    rcb = NULL;
	    ccb->msg.msg_active = FALSE;	/* Entire message flushed */
	}
    }

    /*
    ** Flush INFO message queue prior to starting new message.
    */
    gcd_flush_info( ccb );

    /*
    ** Allocate new buffer (if needed) 
    ** and initialize message header.
    */
    if ( ! rcb )  rcb = gcd_new_rcb( ccb, -1 );
    if ( rcb )  gcd_hdr_init( rcb, msg_id );

    *rcb_ptr = rcb;
    return( rcb != NULL );
}


/*
** Name: gcd_msg_end
**
** Description:
**	Complete and optionally send a message started with
**	gcd_msg_begin().  Message will also be sent when
**	required by the connection state.  In all cases, the
**	RCB reference returned will be suitable as input to
**	gcd_msg_begin().
**
** Input:
**	rcb_ptr	Request control block.
**	flush	TRUE if message is to be sent, FALSE otherwise.
**
** Output:
**	rcb_ptr	Request control block or NULL.
**
** Returns:
**	void
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	15-Jan-03 (gordy)
**	    Implemented TL Service Provider interface.  Adjusted
**	    semantics of RCB buffer parameters.
**	21-Apr-06 (gordy)
**	    RCB buffer reference now passed as parameters so that
**	    the correct reference can be returned.
**	25-Mar-10 (gordy)
**	    Converted as cover for gcd_msg_complete().
*/

void
gcd_msg_end( GCD_RCB **rcb_ptr, bool flush )
{
    gcd_msg_complete( rcb_ptr, flush ? MSG_HDR_EOG : 0 );
    return;
}


/*
** Name: gcd_msg_complete
**
** Description:
**	Complete a message started with gcd_msg_begin().  
**	Message is marked as EOD along with any other
**	header flags provided.  Message will be sent if 
**	EOG or if required by the connection state.  In 
**	all cases, the RCB reference returned will be 
**	suitable as input to gcd_msg_begin().
**
** Input:
**	rcb_ptr	Request control block.
**	flags	Message header flags.
**
** Output:
**	rcb_ptr	Request control block or NULL.
**
** Returns:
**	void
**
** History:
**	25-Mar-10 (gordy)
**	    Extracted from gcd_msg_end().
*/

void
gcd_msg_complete( GCD_RCB **rcb_ptr, u_i1 flags )
{
    GCD_RCB	*rcb = *rcb_ptr;
    GCD_CCB	*ccb;
	
    if ( ! (rcb  &&  rcb->msg.msg_id) )		/* No current message */
    	return;

    /*
    ** Update final message segment.
    */
    gcd_hdr_upd( rcb, MSG_HDR_EOD | flags );
    ccb = rcb->ccb;

    /*
    ** Mark the RCB message as complete.  Ending a 
    ** message does not necessarily cause the message 
    ** to be inactive.  Only once the entire message 
    ** has been flushed to the communications channel 
    ** is the message actually inactive.
    */
    rcb->msg.msg_id = 0;

    /*
    ** The current buffer is sent if INFO messages have
    ** been queued so that the info queue can be flushed.
    ** EOG also forces the buffer to be sent.
    */
    if ( (flags & MSG_HDR_EOG)  ||  ! QUEUE_EMPTY( &ccb->msg.info_q ) )
    {
	/*
	** Send the message.  The RCB is simply freed if the
	** TL service has been terminated.
	*/
        if ( ccb->msg.tl_service )  
	    (*ccb->msg.tl_service->send)( rcb );
	else
	    gcd_del_rcb( rcb );

	*rcb_ptr = NULL;
        ccb->msg.msg_active = FALSE;	/* Entire message sent */
    }

    gcd_flush_info( ccb );
    return;
}


/*
** Name: gcd_msg_split
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
**	15-Jan-03 (gordy)
**	    Implemented TL Service Provider interface.  Adjusted
**	    semantics of RCB buffer parameters.
**	21-Apr-06 (gordy)
**	    The buffer is now allocated by gcd_msg_begin().
**	    Flag active message in communication channel.
*/

bool
gcd_msg_split( GCD_RCB **rcb_ptr )
{
    GCD_RCB	*rcb = *rcb_ptr;
    GCD_CCB	*ccb;
    u_i1	msg_id;

    if ( ! (rcb  &&  rcb->msg.msg_id) )		/* No current message */
        return( TRUE );

    /*
    ** Update current message segment.
    */
    gcd_hdr_upd( rcb, 0 );
    msg_id = rcb->msg.msg_id;
    rcb->msg.msg_id = 0;
    ccb = rcb->ccb;

    /*
    ** When a message is split, the sending of any other
    ** message must be delayed until the current message
    ** is complete.  Simply beginning a message does not
    ** make the message active (other than within the
    ** specific RCB).  Flushing the first portion of the
    ** message to the communications channel actually
    ** activates the message.
    */
    ccb->msg.msg_active = TRUE;

    /*
    ** Send the message.  The RCB is simply freed if the
    ** TL service has been terminated.
    */
    if ( ccb->msg.tl_service )  
        (*ccb->msg.tl_service->send)( rcb );
    else
	gcd_del_rcb( rcb );

    /*
    ** Initialize the next message segment.
    */
    if ( (rcb = gcd_new_rcb( ccb, -1 )) )
    	gcd_hdr_init( rcb, msg_id );

    *rcb_ptr = rcb;
    return( rcb != NULL );
}


/*
** Name: gcd_hdr_init
**
** Description:
**	Initialize message header.
**
** Input:
**	rcb	Request Control Block.
**	msg_id	Message ID.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	21-Apr-06 (gordy)
**	    Created.
*/

static void
gcd_hdr_init( GCD_RCB *rcb, u_i1 msg_id )
{
    GCD_CCB	*ccb = rcb->ccb;
    DAM_ML_HDR	hdr;
    STATUS	status;

    hdr.hdr_id = ccb->msg.msg_hdr_id;
    hdr.msg_id = msg_id;
    hdr.msg_len = 0;
    hdr.msg_flags = 0;

    rcb->msg.msg_id = msg_id;
    rcb->msg.msg_hdr_pos = rcb->buf_len;

    CV2N_I4_MACRO( &hdr.hdr_id, &rcb->buf_ptr[ rcb->buf_len ], &status );
    rcb->buf_len += CV_N_I4_SZ;

    CV2N_I2_MACRO( &hdr.msg_len, &rcb->buf_ptr[ rcb->buf_len ], &status );
    rcb->buf_len += CV_N_I2_SZ;

    CV2N_I1_MACRO( &hdr.msg_id, &rcb->buf_ptr[ rcb->buf_len ], &status );
    rcb->buf_len += CV_N_I1_SZ;

    CV2N_I1_MACRO( &hdr.msg_flags, &rcb->buf_ptr[ rcb->buf_len ], &status );
    rcb->buf_len += CV_N_I1_SZ;

    return;
}


/*
** Name: gcd_hdr_upd
**
** Description:
**	Update message header with current length and status flags.
**
** Input:
**	rcb	Request control block.
**	flags	Message status flags.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	21-Apr-06 (gordy)
**	    Created.
**	25-Mar-10 (gordy)
**	    Added end-of-batch message header flag.
*/

static void
gcd_hdr_upd( GCD_RCB *rcb, u_i1 flags )
{
    u_i2	len = rcb->buf_len - rcb->msg.msg_hdr_pos - MSG_HDR_LEN;
    STATUS	status;

    if ( GCD_global.gcd_trace_level >= 2 )
	TRdisplay( "%4d    GCD sending message %s: length %d%s%s%s\n", 
		   rcb->ccb->id, gcu_lookup( msgMap, rcb->msg.msg_id ), len,
		   (flags & MSG_HDR_EOD) ? " EOD" : "",
		   (flags & MSG_HDR_EOB) ? " EOB" : "",
		   (flags & MSG_HDR_EOG) ? " EOG" : "" );

    CV2N_I2_MACRO( &len, &rcb->buf_ptr[ rcb->msg.msg_hdr_pos + CV_N_I4_SZ ], 
    		   &status );
    CV2N_I1_MACRO( &flags, 
    		   &rcb->buf_ptr[ rcb->msg.msg_hdr_pos + 
				  CV_N_I4_SZ + CV_N_I2_SZ + CV_N_I1_SZ ], 
		   &status );
    return;
}


/*
** Name: gcd_msg_version
**
** Description:
**	Gets the API version for each client message protocol level.
**
** Input:
**	msg_proto	client message protocol level.
**
** Output:
**	None.
**
** Returns:
** 	API version. Returns 0 for invalid message protocol.
**
** History:
**	20-Nov-07 (rajus01) Bug 119505, SD Issue: 122906
**	    Created.
**	26-Oct-09 (gordy)
**	    Added protocol level 7.
*/
u_i2
gcd_msg_version ( u_i2 msg_proto )
{
     switch( msg_proto )
     {
        case MSG_PROTO_1:
            return IIAPI_VERSION_2;
        case MSG_PROTO_2:
            return IIAPI_VERSION_3;
        case MSG_PROTO_3:
        case MSG_PROTO_4:
            return IIAPI_VERSION_4;
        case MSG_PROTO_5:
            return IIAPI_VERSION_5;
        case MSG_PROTO_6:
            return IIAPI_VERSION_6;
	case MSG_PROTO_7:
	    return IIAPI_VERSION_8;
        default:
            break;
     }

     return 0;
}
