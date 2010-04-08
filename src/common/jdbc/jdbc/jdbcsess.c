/*
** Copyright (c) 2004 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <er.h>
#include <me.h>
#include <qu.h>
#include <st.h>
#include <tr.h>

#include <iicommon.h>
#include <gcu.h>
#include <jdbc.h>
#include <jdbcapi.h>
#include <jdbcmsg.h>

/*
** Name: jdbcsess.c
**
** Description:
**	JDBC client session processing and message buffer
**	access functions.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	10-Sep-99 (gordy)
**	    Parameterized initial session info.
**	14-Sep-99 (gordy)
**	    Implemented JDBC error messages.
**	29-Sep-99 (gordy)
**	    Implemented support for BLOB data.
**	26-Oct-99 (gordy)
**	    Added jdbc_find_cursor() to support 
**	    cursor positioned DELETE/UPDATE.
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
**	19-May-00 (gordy)
**	    Statements which are still active (ccb->api.stmt)
**	    need to be cancelled before being closed. 
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-Oct-00 (gordy)
**	    Callbacks now maintained on stack.  Reworked
**	    jdbc_del_stmt() to use stack for nested request.
**	    Added jdbc_purge_stmt().  Allocate RCB in
**	    jdbc_sess_error() if needed (CCB added to params).
**	19-Oct-00 (gordy)
**	    Added jdbc_active_stmt().
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
*/	

GLOBALREF GCULIST msgMap[];

static	void	sess_abort_cmpl( PTR arg );
static	void	cancel_cmpl( PTR arg );
static	void	purge_cmpl( PTR arg );

static	STATUS	build_msg_list( JDBC_RCB * );
static	bool	read_msg_hdr( JDBC_RCB * );



/*
** Name: jdbc_sess_begin
**
** Description:
**	Begin a new JDBC session.  Validates and negotiates
**	connection parameters.
**
** Input:
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
*/

STATUS
jdbc_sess_begin( JDBC_CCB *ccb, u_i1 **params, u_i2 *length )
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
	    if ( JDBC_global.trace_level >= 1 )
		TRdisplay( "%4d    JDBC no extended length indicator (%d)\n",
			    ccb->id, plen );
	    return( E_JD010A_PROTOCOL_ERROR );
	}

	if ( pl2 > plen )  
	{
	    if ( JDBC_global.trace_level >= 1 )
		TRdisplay( "%4d    JDBC parameter too long (%d,%d)\n",
			    ccb->id, pl2, plen );
	    return( E_JD010A_PROTOCOL_ERROR );
	}

	switch( param_id )
	{
	case JDBC_MSG_P_PROTO :
	    if ( pl2 == CV_N_I1_SZ )
		CV2L_I1_MACRO( pbuff, &ccb->conn_info.proto_lvl, &status );
	    else
	    {
		if ( JDBC_global.trace_level >= 1 )
		    TRdisplay( "%4d    JDBC invalid PROTO length %d\n",
				ccb->id, pl2 );
		return( E_JD010A_PROTOCOL_ERROR );
	    }
	    break;

	case JDBC_MSG_P_DTMC :
	    if ( pl2 != 0 )
	    {
		if ( JDBC_global.trace_level >= 1 )
		    TRdisplay( "%4d    JDBC invalid DTMC length %d\n",
				ccb->id, pl2 );
		return( E_JD010A_PROTOCOL_ERROR );
	    }

	    ccb->conn_info.dtmc = TRUE;
	    break;

	default :
	    if ( JDBC_global.trace_level >= 1 )
		TRdisplay( "%4d    JDBC unknown session parameter ID: %d\n",
			    ccb->id, param_id );
	    break;
	}

	pbuff += pl2;
	plen -= pl2;
    }

    if ( plen > 0 )  
    {
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC extraneous parameter data (%d bytes)\n",
			ccb->id, plen );
	return( E_JD010A_PROTOCOL_ERROR );
    }

    if ( ccb->conn_info.proto_lvl < JDBC_MSG_PROTO_1 )
    {
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC Invalid protocol level %d\n",
			ccb->id, ccb->conn_info.proto_lvl );
	return( E_JD010A_PROTOCOL_ERROR );
    }
    
    /*
    ** Negotiate connection parameters.
    */
    if ( ccb->conn_info.proto_lvl > JDBC_MSG_DFLT_PROTO )
	ccb->conn_info.proto_lvl = JDBC_MSG_DFLT_PROTO;
    
    if ( JDBC_global.trace_level >= 2 )
	TRdisplay( "%4d    JDBC New connection (proto lvl %d)\n", 
		   ccb->id, ccb->conn_info.proto_lvl );

    pbuff = ccb->conn_info.params;
    plen = 0;
    param_id = JDBC_MSG_P_PROTO;
    CV2N_I1_MACRO( &param_id, &pbuff[ plen ], &status );
    plen += CV_N_I1_SZ;

    pl1 = 1;
    CV2N_I1_MACRO( &pl1, &pbuff[ plen ], &status );
    plen += CV_N_I1_SZ;

    CV2N_I1_MACRO( &ccb->conn_info.proto_lvl, &pbuff[ plen ], &status );
    plen += CV_N_I1_SZ;

    if ( ccb->conn_info.dtmc )
    {
	param_id = JDBC_MSG_P_DTMC;
	CV2N_I1_MACRO( &param_id, &pbuff[ plen ], &status );
	plen += CV_N_I1_SZ;

	pl1 = 0;
	CV2N_I1_MACRO( &pl1, &pbuff[ plen ], &status );
	plen += CV_N_I1_SZ;
    }

    *params = pbuff;
    *length = plen;

    /*
    ** Set the max message data length which is the
    ** max message length minus the message header.
    */
    ccb->max_data_len = ccb->max_mesg_len - JDBC_MSG_HDR_LEN;

    return( OK );
}


/*
** Name: jdbc_sess_process
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
**	    Added flow control to GCC/MSG sub-systems.
*/

void
jdbc_sess_process( JDBC_RCB *rcb )
{
    JDBC_CCB	*ccb = rcb->ccb;
    STATUS	status;

    /*
    ** Add new message segments to connection message list.
    */
    if ( (status = build_msg_list( rcb )) != OK )
    {
	jdbc_del_rcb( rcb );
	jdbc_gcc_abort( ccb, status );
	return;
    }

    /*
    ** Notify the messaging sub-system that there is a
    ** message ready to be processed.  If messaging is
    ** active (couldn't notify), pause the inbound IO
    ** until processing can catch up.
    */
    if ( ! jdbc_msg_notify( ccb ) )  jdbc_gcc_xoff( ccb, TRUE );
    return;
}


/*
** Name: jdbc_sess_interrupt
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
*/

void
jdbc_sess_interrupt( JDBC_CCB *ccb )
{
    JDBC_PCB *pcb;

    if ( ! (pcb = jdbc_new_pcb( ccb, FALSE )) )
    {
	jdbc_gcc_abort( ccb, E_JD0108_NO_MEMORY );
	return;
    }

    /*
    ** No callback is pushed, so the PCB will be
    ** deleted when the cancel operation completes.
    */
    jdbc_api_cancel( pcb );
    return;
}

/*
** Name: jdbc_sess_end
**
** Description:
**	End a JDBC session.  Releases the connection
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
**	 3-Jun-99 (gordy)
**	    Created.
**	17-Jan-00 (gordy)
**	    Connection handle moved to info block for pooling.
**	 1-Mar-00 (gordy)
**	    Transaction info moved to CIB.
**	11-Oct-00 (gordy)
**	    Callbacks now maintained on stack.
*/

void
jdbc_sess_end( JDBC_CCB *ccb )
{
    JDBC_PCB	*pcb;

    if ( ccb->cib  &&  ccb->cib->conn )
    {
	/*
	** Since we have not initiated shutting down the
	** DBMS connection prior to this point, something
	** bad must have happened.  We will abort the
	** connection to ensure everything is cleaned up.
	*/
	if ( JDBC_global.trace_level >= 2 )
	    TRdisplay( "%4d    JDBC aborting connection\n", ccb->id );

	if ( ! (pcb = jdbc_new_pcb( ccb, FALSE )) )
	{
	    /*
	    ** There is no way to drop the connection
	    ** to the server.  Clean-up the connection
	    ** to the client the best we can.
	    */
	    ccb->cib->conn = ccb->cib->tran = NULL;
	    jdbc_del_ccb( ccb );
	    return;
	}

	pcb->data.disc.conn = ccb->cib->conn;
	ccb->cib->conn = ccb->cib->tran = NULL;
	jdbc_push_callback( pcb, sess_abort_cmpl );
	jdbc_api_abort( pcb );
    }
    else
    {
	/*
	** DBMS session is closed or in the process
	** of being closed.
	*/
	if ( JDBC_global.trace_level >= 2 )
	    TRdisplay( "%4d    JDBC connection closed\n", ccb->id );
    }

    return;
}

static void
sess_abort_cmpl( PTR arg )
{
    JDBC_PCB	*pcb = (JDBC_PCB *)arg;

    jdbc_del_ccb( pcb->ccb );
    jdbc_del_pcb( pcb );

    return;
}


/*
** Name: jdbc_new_stmt
**
** Description:
**	Create a statement control block for the current API
**	statement.  Statements generally represent an open
**	cursor, but temporary statements for select loops may
**	be requested.
**
**	For non-temporary statements:
**	  * Save the current API statement (on the ccb) for later access.  
**	  * Generates a unique ID for the statement and places the ID in 
**	    the pcb as a result value.
**
** Input:
**	ccb		Connection control block.
**	pcb		Parameter control block.
**	 data
**	  desc
**	   count	Number of columns.
**	   desc		Column descriptors.
**
** Output:
**	pcb
**	 result	
**	  stmt_id	Statement ID.
**
** Returns:
**	bool	TRUE if successful, FALSE otherwise.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented support for BLOB data.
**	17-Jan-00 (gordy)
**	    Allow temporary (non-cursor) statements to be created.
*/

JDBC_SCB *
jdbc_new_stmt( JDBC_CCB *ccb, JDBC_PCB *pcb, bool temp )
{
    JDBC_SCB	*scb;

    if ( ! (scb = (JDBC_SCB *)MEreqmem( 0, sizeof( JDBC_SCB ), TRUE, NULL )) )
    {
	gcu_erlog(0, JDBC_global.language, E_JD0108_NO_MEMORY, NULL, 0, NULL);
	return( NULL );
    }

    if ( ! jdbc_alloc_qdesc( &scb->column, pcb->data.desc.count, 
			     (PTR)pcb->data.desc.desc ) )
    {
	MEfree( (PTR)scb );
	return( NULL );
    }

    if ( ! jdbc_alloc_qdata( &scb->column, max(1, pcb->result.fetch_limit) ) )
    {
	jdbc_free_qdata( &scb->column );
	MEfree( (PTR)scb );
	return( NULL );
    }

    scb->stmt_id.id_high = ccb->xact.tran_id;
    scb->stmt_id.id_low = ++ccb->stmt.stmt_id;
    scb->seg_max = ccb->max_data_len;
    scb->handle = ccb->api.stmt;
    ccb->api.stmt = NULL;

    if ( temp )
	scb->crsr_name[0] = EOS;
    else
    {
	MEcopy( (PTR)ccb->qry.crsr_name, 
		sizeof( scb->crsr_name ), (PTR)scb->crsr_name );

	pcb->result.flags |= PCB_RSLT_STMT_ID;
	pcb->result.stmt_id.id_high = scb->stmt_id.id_high;
	pcb->result.stmt_id.id_low = scb->stmt_id.id_low;
    }

    QUinsert( &scb->q, &ccb->stmt.stmt_q );

    if ( JDBC_global.trace_level >= 5 )
	TRdisplay( "%4d    JDBC new STMT (%d,%d)\n", 
		   ccb->id, scb->stmt_id.id_high, scb->stmt_id.id_low );

    return( scb );
}


/*
** Name: jdbc_del_stmt
**
** Descriptions:
**	Close a statement associated with a connection.
**	and free its resources.  A callback function is
**	called from the PCB stack when complete.
**	
** Input:
**	pcb	Parameter control block.
**	scb	Statement control block.
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
**	29-Sep-99 (gordy)
**	    Implemented support for BLOB data.
**	19-May-00 (gordy)
**	    Statements which are still active (ccb->api.stmt)
**	    need to be cancelled before being closed.  This
**	    is especially true for execution of select loops
**	    which do not become inactive until the last tuple
**	    is received.
**	11-Oct-00 (gordy)
**	    Nested request now handled through stack rather than
**	    extra PCB (in SCB).  Since handle in callback is
**	    available in PCB, SCB is now freed before any API
**	    operations.  The call to jdbc_api_close() does the
**	    jdbc_pop_callback() of the callback required by
**	    this routine.
*/

void
jdbc_del_stmt( JDBC_PCB *pcb, JDBC_SCB *scb )
{
    JDBC_CCB	*ccb = pcb->ccb;
    PTR		handle = scb->handle;

    if ( JDBC_global.trace_level >= 5 )
	TRdisplay( "%4d    JDBC del STMT (%d,%d)\n", ccb->id, 
		   scb->stmt_id.id_high, scb->stmt_id.id_low );

    QUremove( &scb->q );
    jdbc_free_qdata( &scb->column );
    if ( scb->seg_buff ) MEfree( (PTR)scb->seg_buff );
    MEfree( (PTR)scb );

    if ( handle != ccb->api.stmt )
	jdbc_api_close( pcb, handle );
    else
    {
	/*
	** The statement is active, so we need to
	** cancel the statement before closing.
	*/
	jdbc_push_callback( pcb, cancel_cmpl );
	jdbc_api_cancel( pcb );
    }

    return;
}

static void
cancel_cmpl( PTR arg )
{
    JDBC_PCB	*pcb = (JDBC_PCB *)arg;
    JDBC_CCB	*ccb = pcb->ccb;
    PTR		handle = ccb->api.stmt;

    /*
    ** Close the active statment which has been cancelled.
    */
    ccb->api.stmt = NULL;
    jdbc_api_close( pcb, handle );

    return;
}


/*
** Name: jdbc_purge_stmt
**
** Description:
**	Delete all statements associated with a connection.
**	Error conditions are signaled with pcb->api_error.
**	A callback function is called from the PCB stack 
**	when complete.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	11-Oct-00 (gordy)
**	    Created.
*/

void
jdbc_purge_stmt( JDBC_PCB *pcb )
{
    JDBC_CCB	*ccb = pcb->ccb;

    pcb->api_error = OK;
    purge_cmpl( (PTR)pcb );
    return;
}

static void
purge_cmpl( PTR arg )
{
    JDBC_PCB	*pcb = (JDBC_PCB *)arg;
    JDBC_CCB	*ccb = pcb->ccb;

    if ( pcb->api_error  ||  QUEUE_EMPTY( &ccb->stmt.stmt_q ) )
	jdbc_pop_callback( pcb );
    else
    {
	jdbc_push_callback( pcb, purge_cmpl );
	jdbc_del_stmt( pcb, (JDBC_SCB *)ccb->stmt.stmt_q.q_next );
    }

    return;
}


/*
** Name: jdbc_active_stmt
**
** Description:
**	Returns and indication that there are active statements
**	associated with the connection.
**
** Input:
**	ccb	Connection control block.
**
** Output:
**	None.
**
** Returns:
**	bool	TRUE if active statements, FALSE otherwise.
**
** History:
**	19-Oct-00 (gordy)
**	    Created.
*/

bool
jdbc_active_stmt( JDBC_CCB *ccb )
{
    return( ! QUEUE_EMPTY( &ccb->stmt.stmt_q ) );
}


/*
** Name: jdbc_find_stmt
**
** Description:
**	Search the connection statement queue for a statement
**	control block with a matching statement ID.
**
** Input:
**	ccb		Connection control block.
**	stmt_id		Statement ID.
**
** Output:
**	None.
**
** Returns:
**	JDBC_SCB *	Statement control block, or NULL if not found.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
*/

JDBC_SCB *
jdbc_find_stmt( JDBC_CCB *ccb, STMT_ID *stmt_id )
{
    JDBC_SCB	*scb;
    QUEUE	*q;

    for(
	 q = ccb->stmt.stmt_q.q_next, scb = NULL;
	 q != &ccb->stmt.stmt_q;
	 q = q->q_next, scb = NULL
       )
    {
	scb = (JDBC_SCB *)q;

	if ( scb->stmt_id.id_high == stmt_id->id_high  &&
	     scb->stmt_id.id_low  == stmt_id->id_low )
	    break;
    }

    if ( ! scb  &&  JDBC_global.trace_level >= 5 )
	TRdisplay( "%4d    JDBC STMT (%d,%d) does not exist!\n", 
		   ccb->id, scb->stmt_id.id_high, scb->stmt_id.id_low );

    return( scb );
}


/*
** Name: jdbc_find_cursor
**
** Description:
**	Search the connection statement queue for a statement
**	control block with a matching cursor name.
**
** Input:
**	ccb		Connection control block.
**	cursor		Cursor name.
**
** Output:
**	None.
**
** Returns:
**	JDBC_SCB *	Statement control block, or NULL if not found.
**
** History:
**	26-Oct-99 (gordy)
**	    Created.
*/

JDBC_SCB *
jdbc_find_cursor( JDBC_CCB *ccb, char *cursor )
{
    JDBC_SCB	*scb;
    QUEUE	*q;

    for(
	 q = ccb->stmt.stmt_q.q_next, scb = NULL;
	 q != &ccb->stmt.stmt_q;
	 q = q->q_next, scb = NULL
       )
    {
	scb = (JDBC_SCB *)q;
	if ( ! STbcompare( cursor, 0, scb->crsr_name, 0, TRUE ) )  break;
    }

    if ( ! scb  &&  JDBC_global.trace_level >= 5 )
	TRdisplay( "%4d    JDBC cursor %s does not exist!\n", 
		   ccb->id, cursor );

    return( scb );
}


/*
** Name: jdbc_sess_error
**
** Description:
**	Retrieves the information for an error code and sends
**	a JDBC Error message to the client.  RCB is allocated
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
*/

void
jdbc_sess_error( JDBC_CCB *ccb, JDBC_RCB **rcb_ptr, STATUS error )
{
    i4		length;
    char	msg[ ER_MAX_LEN ];
    char	sqlState[ DB_SQLSTATE_STRING_LEN ];
    STATUS	status;
    CL_ERR_DESC	cl_err;

    if ( ! *rcb_ptr  &&  ! (*rcb_ptr = jdbc_new_rcb( ccb )) )
    {
	jdbc_gcc_abort( ccb, E_JD0108_NO_MEMORY );
	return;
    }

    status = ERslookup( error, NULL, ER_TEXTONLY, sqlState, msg, sizeof( msg ),
			JDBC_global.language, &length, &cl_err, 0, NULL );

    if ( status != OK )
    {
	CL_ERR_DESC	junk;
	i4		len;

	STprintf( msg, "Couldn't look up message %x (ER error %x)\n",
		  error, status );
	length = STlength( msg );

	status = ERslookup( status, &cl_err, 0, NULL, 
			    &msg[ length ], sizeof( msg ) - length,
			    JDBC_global.language, &len, &junk, 0, NULL );
	if ( status == OK )  length += len;
    }

    jdbc_send_error( rcb_ptr, JDBC_ET_ERR, error, sqlState, (u_i2)length, msg );
    return;
}


/*
** Name: build_msg_list
**
** Description:
**	Process a request control block by separating
**	message segments into individual control blocks
**	and placing the blocks on the connection message
**	list for simplified access.
**
**	The RCB is not freed if an error occurs and is
**	left to the calling routine to handle.  For a
**	successful call, the RCB will be freed when the
**	contents have been processed.
**
** Input:
**	rcb	Request control block.
**
** Output:
**	None.
**
** Returns:
**	STATUS	OK or error code.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
*/

static STATUS
build_msg_list( JDBC_RCB *rcb )
{
    /*
    ** Skip empty NL/TL data segments.
    */
    if ( ! rcb->buf_len )
    {
	/*
	** The caller won't free the RCB, so we
	** do it here since the buffer is empty.
	*/
	jdbc_del_rcb( rcb );
	return( OK );
    }

    for(;;)
    {
	if ( ! read_msg_hdr( rcb ) )  return( E_JD010A_PROTOCOL_ERROR );

	if ( JDBC_global.trace_level >= 2 )
	    TRdisplay( "%4d    JDBC received message %s length %d%s%s\n", 
		       rcb->ccb->id, gcu_lookup( msgMap, rcb->msg.msg_id ), 
		       rcb->msg.msg_len,
		       rcb->msg.msg_flags & JDBC_MSGFLG_EOD ? " EOD" : "",
		       rcb->msg.msg_flags & JDBC_MSGFLG_EOG ? " EOG" : "" );

	/*
	** If the JDBC client splits a message it is
	** probably because it exceeds the negotiated
	** buffer size.  Multiple message segments in
	** a single buffer is most likely caused by
	** different message types.  In either case
	** each individual message segment is placed
	** into its own RCB and linked together.
	*/
        if ( rcb->msg.msg_len < rcb->buf_len )
	{
	    JDBC_RCB	*new_rcb;

	    if ( ! (new_rcb = jdbc_new_rcb( rcb->ccb )) )
		return( E_JD0108_NO_MEMORY );

	    /*
	    ** Extract leading message segment into new RCB.
	    */
	    new_rcb->msg.msg_id = rcb->msg.msg_id;
	    new_rcb->msg.msg_len = rcb->msg.msg_len;
	    new_rcb->msg.msg_flags = rcb->msg.msg_flags;
	    MEcopy( &rcb->buffer[ rcb->buf_ptr ], 
		    rcb->msg.msg_len, &new_rcb->buffer[ new_rcb->buf_ptr ] );
	    new_rcb->buf_len += rcb->msg.msg_len;
	    rcb->buf_ptr += rcb->msg.msg_len;
	    rcb->buf_len -= rcb->msg.msg_len;

	    QUinsert( &new_rcb->q, rcb->ccb->msg.msg_q.q_prev );
	    continue;
	}

	/*
	** RCB now has just a single message segment.
	** Append to end of list and we are done.
	*/
	QUinsert( &rcb->q, rcb->ccb->msg.msg_q.q_prev );
	break;
    }

    return( OK );
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
*/

static bool
read_msg_hdr( JDBC_RCB *rcb )
{
    JDBC_MSG_HDR    hdr;
    STATUS	    status;

    /*
    ** Headers may not be split.
    */
    if ( rcb->buf_len < JDBC_MSG_HDR_LEN )
    {
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC insufficient data for message header\n", 
		       rcb->ccb->id );
	return( FALSE );
    }

    CV2L_I4_MACRO( &rcb->buffer[ rcb->buf_ptr ], &hdr.hdr_id, &status );
    rcb->buf_ptr += CV_N_I4_SZ;
    rcb->buf_len -= CV_N_I4_SZ;

    CV2L_I2_MACRO( &rcb->buffer[ rcb->buf_ptr ], &hdr.msg_len, &status );
    rcb->buf_ptr += CV_N_I2_SZ;
    rcb->buf_len -= CV_N_I2_SZ;

    CV2L_I1_MACRO( &rcb->buffer[ rcb->buf_ptr ], &hdr.msg_id, &status );
    rcb->buf_ptr += CV_N_I1_SZ;
    rcb->buf_len -= CV_N_I1_SZ;

    CV2L_I1_MACRO( &rcb->buffer[ rcb->buf_ptr ], &hdr.msg_flags, &status );
    rcb->buf_ptr += CV_N_I1_SZ;
    rcb->buf_len -= CV_N_I1_SZ;

    /*
    ** Validate header info.
    */
    if ( hdr.hdr_id != JDBC_MSG_HDR_ID )
    {
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC invalid message header ID: 0x%x\n", 
		       rcb->ccb->id, hdr.hdr_id );
	return( FALSE );
    }

    /*
    ** A message segment should be contained
    ** within a NL/TL data segment.  NL/TL
    ** are required to provide an entire data
    ** segment even if split during transport.
    */
    if ( rcb->msg.msg_len > rcb->buf_len )
    {
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC insufficient data message segment\n",
			rcb->ccb->id );
	return( FALSE );
    }

    rcb->msg.msg_id = hdr.msg_id;
    rcb->msg.msg_len = hdr.msg_len;
    rcb->msg.msg_flags = hdr.msg_flags;

    return( TRUE );
}

