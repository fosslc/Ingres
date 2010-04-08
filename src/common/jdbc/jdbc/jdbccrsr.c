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
** Name: jdbccrsr.c
**
** Description:
**	JDBC cursor message processing.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	14-Sep-99 (gordy)
**	    Implemented JDBC error messages.
**	18-Oct-99 (gordy)
**	    Extracted to this file.
**	 4-Nov-99 (gordy)
**	    Updated calling sequence of jdbc_send_done() to allow
**	    control of ownership of RCB parameter.
**	13-Dec-99 (gordy)
**	    Added support for cursor pre-fetch.
**	19-May-00 (gordy)
**	    Separate request/result row counters to better
**	    detect end-of-tuples.  Only deactivate a statement
**	    when end-of-tuples detected or for cursors at the
**	    end of the fetch.
**	 3-Aug-00 (gordy)
**	    Get query info when statement goes dormant to pick
**	    up the query results.
**	11-Oct-00 (gordy)
**	    Callbacks now maintained on stack.
**	19-Oct-00 (gordy)
**	    Signal cursor close to transaction sub-system to
**	    allow handling of autocommit simulation.
**	10-May-01 (gordy)
**	    Added support for UCS2 datatypes.
**	 2-Jul-01 (gordy)
**	    Make sure UCS2 characters are not split across buffers.
**	13-Nov-01 (loera01) SIR 106366
**	    Conditionally compile Unicode support.
**	    BACKED OUT, NOT NEEDED IN MAIN. (hanje04)
*/	

/*
** Cursor sequencing.
*/

static	GCULIST curMap[] =
{
    { JDBC_CUR_CLOSE,	"CLOSE" },
    { JDBC_CUR_FETCH,	"FETCH" },
    { 0, NULL }
};

static	void	msg_fetch( JDBC_SCB *, JDBC_PCB * );
static	void	msg_clos_cmpl( PTR );
static	void	msg_ftch_cmpl( PTR );
static	void	msg_ftch_done( PTR );
static	void	send_data( JDBC_CCB *, JDBC_SCB *, JDBC_RCB **, u_i2, bool );


/*
** Name: jdbc_msg_cursor
**
** Description:
**	Process a JDBC cursor message.
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
**	    Renamed for external reference.
**	13-Dec-99 (gordy)
**	    Check for the cursor pre-fetch parameter.
**	19-May-00 (gordy)
**	    Separate request/result row counters to better
**	    detect end-of-tuples.
**	11-Oct-00 (gordy)
**	    Callbacks now maintained on stack.
*/

void
jdbc_msg_cursor( JDBC_CCB *ccb )
{
    JDBC_MSG_CUR	cursor;
    STMT_ID		stmt_id;
    JDBC_SCB		*scb;
    JDBC_PCB		*pcb;
    STATUS		status;
    u_i4		pre_fetch = 1;
    bool		incomplete = FALSE;
    bool		got_id = FALSE;

    if ( ! jdbc_get_i2( ccb, (u_i1 *)&cursor.cursor_op ) )
    {
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC no cursor op specified\n", ccb->id );
	jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
	return;
    }

    if ( JDBC_global.trace_level >= 3 )
	TRdisplay( "%4d    JDBC Cursor request: %s\n", 
		   ccb->id, gcu_lookup( curMap, cursor.cursor_op ) );

    if ( ! (pcb = jdbc_new_pcb( ccb, TRUE )) )
    {
	jdbc_gcc_abort( ccb, E_JD0108_NO_MEMORY );
	return;
    }

    while( ccb->msg.msg_len )
    {
	JDBC_MSG_PM	cp;
	u_i1		u_i1_val;
	u_i2		u_i2_val;
	u_i4		u_i4_val;

	incomplete = TRUE;
	if ( ! jdbc_get_i2( ccb, (u_i1 *)&cp.param_id ) )  break;
	if ( ! jdbc_get_i2( ccb, (u_i1 *)&cp.val_len ) )  break;

	switch( cp.param_id )
	{
	case JDBC_CUR_STMT_ID :
	    if ( cp.val_len != (CV_N_I4_SZ * 2)  ||
	         ! jdbc_get_i4( ccb, (u_i1 *)&stmt_id.id_high )  ||
		 ! jdbc_get_i4( ccb, (u_i1 *)&stmt_id.id_low ) )
		break;

	    incomplete = FALSE;
	    got_id = TRUE;
	    break;
	
	case JDBC_CUR_PRE_FETCH :
	    switch( cp.val_len )
	    {
	    case 1 : 
		if ( jdbc_get_i1( ccb, (u_i1 *)&u_i1_val ) )
		{
		    pre_fetch = u_i1_val;
		    incomplete = FALSE;
		}
		break;

	    case 2 :
		if ( jdbc_get_i2( ccb, (u_i1 *)&u_i2_val ) )
		{
		    pre_fetch = u_i2_val;
		    incomplete = FALSE;
		}
		break;

	    case 4 :
		if ( jdbc_get_i4( ccb, (u_i1 *)&u_i4_val ) )
		{
		    pre_fetch = u_i4_val;
		    incomplete = FALSE;
		}
		break;
	    }
	    break;

	default :
	    if ( JDBC_global.trace_level >= 3 )
		TRdisplay( "%4d    JDBC     unkown parameter ID %d\n",
			   ccb->id, cp.param_id );
	    break;
	}

	if ( incomplete )  break;
    }

    if ( incomplete )
    {
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC unable to read all cursor parameters %d\n",
		       ccb->id );
	status = E_JD010A_PROTOCOL_ERROR;
    }
    else  if ( ! got_id )
    {
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC no cursor ID for cursor message\n", 
			ccb->id );
	status = E_JD010A_PROTOCOL_ERROR;
    }
    else
	status = OK;

    if ( status != OK )
    {
	jdbc_gcc_abort( ccb, status );
	jdbc_del_pcb( pcb );
	return;
    }

    if ( ! (scb = jdbc_find_stmt( ccb, &stmt_id )) )
    {
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC no active statement for cursor ID\n", 
			ccb->id );
	jdbc_sess_error( ccb, &pcb->rcb, E_JD0111_NO_STMT );
	jdbc_send_done( ccb, &pcb->rcb, pcb );
	jdbc_del_pcb( pcb );
	jdbc_msg_pause( ccb, TRUE );
	return;
    }

    switch( cursor.cursor_op )
    {
    case JDBC_CUR_CLOSE :
	ccb->sequence = 0;
	jdbc_push_callback( pcb, msg_clos_cmpl );
	jdbc_del_stmt( pcb, scb );
	break;
    
    case JDBC_CUR_FETCH :
	if ( scb->column.cur_col != 0  ||  scb->column.more_segments )
	{
	    if ( JDBC_global.trace_level >= 1 )
		TRdisplay("%4d    JDBC previous fetch not complete: (%d,%d)\n",
			  ccb->id, scb->column.cur_col, scb->column.max_cols);
	    jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
	    jdbc_del_pcb( pcb );
	    return;
	}

	ccb->api.stmt = scb->handle;
	pcb->scb = scb;
	pcb->data.data.max_row = max(1, min(pre_fetch, scb->column.max_rows));
	msg_fetch( scb, pcb );
	break;

    default :
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC invalid cursor op: %d\n", 
			ccb->id, cursor.cursor_op );
	jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
	jdbc_del_pcb( pcb );
	return;
    }

    return;
}


/*
** Name: msg_clos_cmpl
**
** Description:
**	Callback function for API statement close requests.
**	Sends DONE message.
**
** Input:
**	arg	Parameter control block.
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
**	19-Oct-00 (gordy)
**	    Signal cursor close to transaction sub-system to
**	    allow handling of autocommit simulation.
*/

static void
msg_clos_cmpl( PTR arg )
{
    JDBC_PCB	*pcb = (JDBC_PCB *)arg;
    JDBC_CCB	*ccb = pcb->ccb;

    switch( ccb->sequence++ )
    {
    case 0 : 
	jdbc_push_callback( pcb, msg_clos_cmpl );
	jdbc_xact_check( pcb, JDBC_XQOP_CURSOR_CLOSE );
	break;
    
    case 1 :
	jdbc_send_done( ccb, &pcb->rcb, pcb );
	jdbc_del_pcb( pcb );
	jdbc_msg_pause( ccb, TRUE );
	break;
    }

    return;
}


/*
** Name: msg_fetch
**
** Description:
**	Begin or continue fetching a row.  Determines sub-set of
**	columns which may be requested, starting with first un-
**	processed column and ending with first BLOB encountered
**	or last column.  BLOBs may only be fetched a segment at
**	a time, and the first segment is fetched with all non-
**	BLOB columns which proceed the BLOB.  BLOB columns are
**	not considered processed until end-of-segments indicator
**	has been read.
**
** Input:
**	scb	Statement control block.
**	pcb	Parameter control block.
**
** output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	13-Dec-99 (gordy)
**	    Check for cases which disable cursor pre-fetching:
**	    BLOBs and partial row fetches following BLOBs.
**	19-May-00 (gordy)
**	    Separate request/result row counters to better
**	    detect end-of-tuples.
**	11-Oct-00 (gordy)
**	    Callbacks now maintained on stack.
**	10-May-01 (gordy)
**	    Added support for UCS2 datatypes.
*/

static void
msg_fetch( JDBC_SCB *scb, JDBC_PCB *pcb )
{
    JDBC_CCB		*ccb = pcb->ccb;
    IIAPI_DESCRIPTOR	*desc = (IIAPI_DESCRIPTOR *)scb->column.desc;
    u_i2		column;

    /*
    ** If there is a BLOB column in the data set,
    ** only fetch upto and including the BLOB (for
    ** the first segment).  Additional segments will 
    ** be fetched individually.  Remaining columns 
    ** will be fetched once the BLOB is done.
    */
    for( 
	 column = scb->column.cur_col; 
	 column < scb->column.max_cols; 
	 column++ 
       )
	if ( desc[ column ].ds_dataType == IIAPI_LVCH_TYPE  ||
	     desc[ column ].ds_dataType == IIAPI_LNVCH_TYPE ||
	     desc[ column ].ds_dataType == IIAPI_LBYTE_TYPE )
	{
	    /* Include BLOB and stop */
	    column++;
	    pcb->data.data.max_row = 1; 	/* BLOBs force single row */
	    break;
	}

    if ( scb->column.cur_col )  pcb->data.data.max_row = 1;  /* Partial row */
    scb->column.col_cnt = column - scb->column.cur_col;
    pcb->data.data.col_cnt = scb->column.col_cnt;
    pcb->data.data.data = 
	&((IIAPI_DATAVALUE *)scb->column.data)[ scb->column.cur_col ];
    jdbc_push_callback( pcb, msg_ftch_cmpl );

    if ( ! (ccb->msg.flags & JDBC_MSG_XOFF) )
	jdbc_api_getCol( pcb );
    else
    {
	if ( JDBC_global.trace_level >= 4 )
	    TRdisplay( "%4d    JDBC fetch delayed pending OB XON\n", ccb->id );

	ccb->msg.xoff_pcb = (PTR)pcb;
    }

    return;
}


/*
** Name: msg_ftch_cmpl
**
** Description:
**	Callback function for API get columns requests.  Formats
**	DATA message.  Data message is started when the first
**	column is a part of the column sub-set processed.  Data
**	message is completed when the last column has completed.
**
**	DONE message is sent when columns completed.  Otherwise,
**	column fetching continues with next column sub-set.
**
** Input:
**	arg	Parameter control block.
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
**	 4-Nov-99 (gordy)
**	    Be sure to drop reference to cursor statement handle
**	    when fetch has completed.
**	13-Dec-99 (gordy)
**	    Process multiple rows for cursor pre-fetch.
**	19-May-00 (gordy)
**	    Only deactivate the statement at the end-of-data,
**	    or for cursor at the end of the fetch.
**	 3-Aug-00 (gordy)
**	    Get query info when statement goes dormant to pick
**	    up the query results.
**	11-Oct-00 (gordy)
**	    Callbacks now maintained on stack.
*/

static void
msg_ftch_cmpl( PTR arg )
{
    JDBC_PCB	*pcb = (JDBC_PCB *)arg;
    JDBC_CCB	*ccb = pcb->ccb;
    JDBC_SCB	*scb = pcb->scb;
    u_i2	row;

    if ( pcb->data.data.row_cnt )
    {
	/*
	** Start the DATA message at first column.
	** Be sure to handle the case where first
	** column is a BLOB which has previously
	** returned segments.
	*/
	if ( ! scb->column.cur_col  &&  ! scb->column.more_segments )
	    jdbc_msg_begin( &pcb->rcb, JDBC_MSG_DATA, 0 );

	/*
	** Process the available columns.  If processing
	** a BLOB, don't skip past the last column until
	** all segments have been sent.
	*/
	if ( JDBC_global.trace_level >= 4 )
	    TRdisplay( "%4d    JDBC Fetch: col %d of %d, %d rows %d cols %s\n", 
		       ccb->id, scb->column.cur_col, scb->column.max_cols,
		       pcb->data.data.row_cnt, scb->column.col_cnt, 
		       pcb->data.data.more_segments ? "(more segments)" : "" );

	for( row = 0; row < pcb->data.data.row_cnt; row++ )
	{
	    send_data(ccb, scb, &pcb->rcb, row, pcb->data.data.more_segments);
	    scb->column.cur_col += scb->column.col_cnt -
				   (scb->column.more_segments ? 1 : 0);

	    /*
	    ** If this was a partial fetch, fetch next set.
	    */
	    if ( scb->column.cur_col < scb->column.max_cols )
	    {
		msg_fetch( scb, pcb );	/* continue */
		return;
	    }

	    /*
	    ** Reset for start of next row.
	    */
	    scb->column.cur_col = 0;
	    pcb->data.data.tot_row++;
	}

	jdbc_msg_end( pcb->rcb, FALSE );
    }

    scb->column.cur_col = scb->column.col_cnt = 0;
    scb->column.more_segments = FALSE;

    /*
    ** If we failed to fetch all the requested rows,
    ** we must be at the end of the tuple stream.
    ** Send an indicator to the client.  We also
    ** make the statement dormant since the end of 
    ** the tuple stream has been reached.
    **
    ** Cursor statements are also made dormant since
    ** there is no active tuple stream between fetches.
    */
    if ( pcb->data.data.tot_row < pcb->data.data.max_row )
    {
	pcb->result.flags |= PCB_RSLT_EOD;
	jdbc_push_callback( pcb, msg_ftch_done );
	jdbc_api_gqi( pcb );
    }
    else  if ( scb->crsr_name[0] != EOS )  
    {
	jdbc_push_callback( pcb, msg_ftch_done );
	jdbc_api_gqi( pcb );
    }
    else
    {
	jdbc_send_done( ccb, &pcb->rcb, pcb );
	jdbc_del_pcb( pcb );
	jdbc_msg_pause( ccb, TRUE );
    }

    return;
}

/*
** Name: msg_ftch_done
**
** Description:
**	Callback function for API get query info request.
**	Sends the DONE message and cleans up after processing.
**
** Input:
**	arg	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 3-Aug-00 (gordy)
**	    Created.
*/

static void
msg_ftch_done( PTR arg )
{
    JDBC_PCB	*pcb = (JDBC_PCB *)arg;
    JDBC_CCB	*ccb = pcb->ccb;

    ccb->api.stmt = NULL;
    jdbc_send_done( ccb, &pcb->rcb, pcb );
    jdbc_del_pcb( pcb );
    jdbc_msg_pause( ccb, TRUE );
    return;
}


/*
** Name: send_data
**
** Description:
**	Format and send a DATA message.  The message
**	is appended to an existing request control block
**	if provided.  Otherwise, a new request control block
**	is created and returned with the message.
**
** Input:
**	ccb		Connection control block.
**	scb		Statement control block.
**	rcb_ptr		Request control block.
**	row		Which data row is being processed.
**	more_segments	Are additional BLOB segments available?
**
** Ouput:
**	rcb_ptr	Request control block.
**
** Returns:
**	void.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	13-Dec-99 (gordy)
**	    Added row parameter for handling cursor pre-fetch.
**	01-Feb-00 (rajus01) 
**	    Client expects date string to include timestamp also. 
**	30-May-00 (gordy & rajus01)
**	    Bug #s 101677,101678: Removed length conditional
**	    statement for date strings.
**	11-Oct-00 (gordy)
**	    Added CCB to jdbc_sess_error() parameters.
**	10-May-01 (gordy)
**	    Added support for UCS2 datatypes.
**	 2-Jul-01 (gordy)
**	    Make sure UCS2 characters are not split across buffers.
*/

static void
send_data
( 
    JDBC_CCB	*ccb, 
    JDBC_SCB	*scb, 
    JDBC_RCB	**rcb_ptr, 
    u_i2	row,
    bool	more_segments 
)
{
    IIAPI_DESCRIPTOR	*desc = (IIAPI_DESCRIPTOR *)scb->column.desc;
    IIAPI_DATAVALUE	*data = &((IIAPI_DATAVALUE *)scb->column.data)[ 
						row * scb->column.max_cols ];
    u_i2		end = scb->column.cur_col + scb->column.col_cnt;
    u_i2		col, len;

    for( col = scb->column.cur_col; col < end; col++ )
    {
	if ( desc[ col ].ds_nullable  &&  data[ col ].dv_null )
	{
	    jdbc_put_i1( rcb_ptr, 0 );	/* No data - NULL value */
	    continue;
	}

	/*
	** Write the data indicator byte, for BLOBs this is
	** only done on the first segment (more_segments is
	** saved below once the segment has been processed,
	** so the saved value is FALSE on the first segment).
	*/
	if ( (desc[ col ].ds_dataType != IIAPI_LVCH_TYPE  &&
	      desc[ col ].ds_dataType != IIAPI_LNVCH_TYPE &&
	      desc[ col ].ds_dataType != IIAPI_LBYTE_TYPE)  ||
	     ! scb->column.more_segments )
	    jdbc_put_i1( rcb_ptr, 1 );

	switch( desc[ col ].ds_dataType )
	{
	case IIAPI_INT_TYPE :
	    switch( desc[ col ].ds_length )
	    {
	    case 1 : jdbc_put_i1p(rcb_ptr, (u_i1 *)data[col].dv_value); break;
	    case 2 : jdbc_put_i2p(rcb_ptr, (u_i1 *)data[col].dv_value); break;
	    case 4 : jdbc_put_i4p(rcb_ptr, (u_i1 *)data[col].dv_value); break;
	    }
	    break;

	case IIAPI_FLT_TYPE :
	    switch( desc[ col ].ds_length )
	    {
	    case 4 : jdbc_put_f4p(rcb_ptr, (u_i1 *)data[col].dv_value); break;
	    case 8 : jdbc_put_f8p(rcb_ptr, (u_i1 *)data[col].dv_value); break;
	    }
	    break;

	case IIAPI_DEC_TYPE :
	    {
		IIAPI_DESCRIPTOR	ddesc;
		IIAPI_DATAVALUE		ddata;
		STATUS			status;
		char			dbuff[ 36 ];

		ddesc.ds_dataType = IIAPI_VCH_TYPE;
		ddesc.ds_nullable = FALSE;
		ddesc.ds_length = sizeof( dbuff );
		ddesc.ds_precision = 0;
		ddesc.ds_scale = 0;
		ddata.dv_null = FALSE;
		ddata.dv_length = sizeof( dbuff );
		ddata.dv_value = dbuff;

		status = jdbc_api_format( ccb, &desc[ col ], &data[ col ], 
					  &ddesc, &ddata );

		if ( status != OK )  
		{
		    /*
		    ** Conversion error.  Send a zero-length 
		    ** string as a place holder in the data 
		    ** stream.  Interrupt the data stream with 
		    ** an error message.
		    */
		    jdbc_put_i2( rcb_ptr, 0 );
		    jdbc_msg_end( *rcb_ptr, FALSE );
		    jdbc_sess_error( ccb, rcb_ptr, status );
		    jdbc_msg_begin( rcb_ptr, JDBC_MSG_DATA, 0 );
		}
		else
		{
		    MEcopy( (PTR)dbuff, 2, (PTR)&len );
		    jdbc_put_i2( rcb_ptr, len );
		    jdbc_put_bytes( rcb_ptr, len, (u_i1 *)&dbuff[2] );
		}
	    }
	    break;

	case IIAPI_MNY_TYPE :
	    {
		IIAPI_DESCRIPTOR	idesc, ddesc;
		IIAPI_DATAVALUE		idata, ddata;
		STATUS			status;
		char			dbuff[ 20 ];
		char			dec[ 8 ];

		/*
		** It would be nice to convert directly to
		** varchar, but they money formatting is
		** nasty.  So we first convert to decimal,
		** then to varchar.
		*/
		idesc.ds_dataType = IIAPI_DEC_TYPE;
		idesc.ds_nullable = FALSE;
		idesc.ds_length = sizeof( dec );
		idesc.ds_precision = 15;
		idesc.ds_scale = 2;
		idata.dv_null = FALSE;
		idata.dv_length = sizeof( dec );
		idata.dv_value = (PTR)&dec;

		ddesc.ds_dataType = IIAPI_VCH_TYPE;
		ddesc.ds_nullable = FALSE;
		ddesc.ds_length = sizeof( dbuff );
		ddesc.ds_precision = 0;
		ddesc.ds_scale = 0;
		ddata.dv_null = FALSE;
		ddata.dv_length = sizeof( dbuff );
		ddata.dv_value = dbuff;

		if ( (status = jdbc_api_format( ccb, &desc[ col ], &data[ col ],
						&idesc, &idata )) != OK  ||
		     (status = jdbc_api_format( ccb, &idesc, &idata, 
						&ddesc, &ddata )) != OK )
		{
		    /*
		    ** Conversion error.  Send a zero-length 
		    ** string as a place holder in the data 
		    ** stream.  Interrupt the data stream with 
		    ** an error message.
		    */
		    jdbc_put_i2( rcb_ptr, 0 );
		    jdbc_msg_end( *rcb_ptr, FALSE );
		    jdbc_sess_error( ccb, rcb_ptr, status );
		    jdbc_msg_begin( rcb_ptr, JDBC_MSG_DATA, 0 );
		}
		else
		{
		    MEcopy( (PTR)dbuff, 2, (PTR)&len );
		    jdbc_put_i2( rcb_ptr, len );
		    jdbc_put_bytes( rcb_ptr, len, (u_i1 *)&dbuff[2] );
		}
	    }
	    break;

	case IIAPI_DTE_TYPE :
	    {
		IIAPI_DESCRIPTOR	ddesc;
		IIAPI_DATAVALUE		ddata;
		STATUS			status;
		char			dbuff[ 36 ];

		ddesc.ds_dataType = IIAPI_VCH_TYPE;
		ddesc.ds_nullable = FALSE;
		ddesc.ds_length = sizeof( dbuff );
		ddesc.ds_precision = 0;
		ddesc.ds_scale = 0;
		ddata.dv_null = FALSE;
		ddata.dv_length = sizeof( dbuff );
		ddata.dv_value = dbuff;

		status = jdbc_api_format( ccb, &desc[ col ], &data[ col ], 
					  &ddesc, &ddata );

		if ( status != OK )  
		{
		    /*
		    ** Conversion error.  Send a zero-length 
		    ** string as a place holder in the data 
		    ** stream.  Interrupt the data stream with 
		    ** an error message.
		    */
		    jdbc_put_i2( rcb_ptr, 0 );
		    jdbc_msg_end( *rcb_ptr, FALSE );
		    jdbc_sess_error( ccb, rcb_ptr, status );
		    jdbc_msg_begin( rcb_ptr, JDBC_MSG_DATA, 0 );
		}
		else
		{
		    MEcopy( (PTR)dbuff, 2, (PTR)&len );
		    jdbc_put_i2( rcb_ptr, len );
		    jdbc_put_bytes( rcb_ptr, len, (u_i1 *)&dbuff[2] );
		}
	    }
	    break;

	case IIAPI_CHA_TYPE :
	case IIAPI_CHR_TYPE :
	case IIAPI_BYTE_TYPE :
	    jdbc_put_bytes( rcb_ptr, desc[ col ].ds_length, 
			    (u_i1 *)data[ col ].dv_value );
	    break;

	case IIAPI_NCHA_TYPE :
	    jdbc_put_ucs2( rcb_ptr, desc[ col ].ds_length / sizeof( UCS2 ),
			   (u_i1 *)data[ col ].dv_value );
	    break;

	case IIAPI_TXT_TYPE :
	case IIAPI_LTXT_TYPE :
	case IIAPI_VCH_TYPE :	
	case IIAPI_VBYTE_TYPE :
	    MEcopy( data[ col ].dv_value, 2, (PTR)&len );
	    jdbc_put_i2( rcb_ptr, len );
	    jdbc_put_bytes( rcb_ptr, len, (u_i1 *)data[ col ].dv_value + 2 );
	    break;

	case IIAPI_NVCH_TYPE :
	    MEcopy( data[ col ].dv_value, 2, (PTR)&len );
	    jdbc_put_i2( rcb_ptr, len );
	    jdbc_put_ucs2( rcb_ptr, len, (u_i1 *)data[ col ].dv_value + 2 );
	    break;

	case IIAPI_LVCH_TYPE :
	case IIAPI_LNVCH_TYPE :
	case IIAPI_LBYTE_TYPE :
	    {
		u_i1	*ptr;
		u_i2	seg_len, chrs, char_size = sizeof( char );
		bool	ucs2 = FALSE;

		if ( desc[ col ].ds_dataType == IIAPI_LNVCH_TYPE )
		{
		    ucs2 = TRUE;
		    char_size = sizeof( UCS2 );
		}

		if ( data[ col ].dv_length < 2 )
		    seg_len = 0;
		else
		{
		    MEcopy( data[ col ].dv_value, 2, (PTR)&seg_len );
		    ptr = (u_i1 *)data[ col ].dv_value + 2;
		    seg_len *= char_size;  /* convert array len to byte len */
		}

		/*
		** Output data as long as there is sufficient
		** data to fill the current message.  Any data
		** remaining is saved until additional data is
		** received or the end of the BLOB is reached.
		**
		** We actually make sure that room for the
		** segment (length indicator and data) and an
		** end-of-segments indicator is left in the
		** message buffer.  This way the end-of-BLOB
		** processing below does not need to worry
		** about splitting the message.
		**
		** The test against the save buffer size is
		** redundent since the buffer should be at 
		** least as big as a message buffer, but we 
		** make the test just to be safe.
		*/
		while( (seg_len + scb->seg_len + 4) > RCB_AVAIL(*rcb_ptr)  ||
		       (seg_len + scb->seg_len) > scb->seg_max )
		{
		    /*
		    ** Can a valid data segment be placed in the buffer?
		    */
		    if ( RCB_AVAIL(*rcb_ptr) >= (2 + char_size)  &&  
			 (seg_len + scb->seg_len) >= char_size )
		    {
			len = min( seg_len + scb->seg_len, 
				   RCB_AVAIL( *rcb_ptr ) - 2 );
			chrs = len / char_size;
			len = chrs * char_size;

			jdbc_put_i2( rcb_ptr, chrs );

			if ( JDBC_global.trace_level >= 5 )
			    TRdisplay( "%4d    JDBC send segment: %d (%d,%d)\n",
					ccb->id, len, scb->seg_len, seg_len );

			/*
			** First, send saved data.
			*/
			if ( scb->seg_len >= char_size )
			{
			    len = min( scb->seg_len, RCB_AVAIL( *rcb_ptr ) );
			    chrs = len / char_size;
			    len = chrs * char_size;

			    if ( ! ucs2 )
				jdbc_put_bytes( rcb_ptr, len, scb->seg_buff );
			    else
				jdbc_put_ucs2( rcb_ptr, chrs, scb->seg_buff );

			    scb->seg_len -= len;
			    if ( scb->seg_len ) 
				MEcopy( (PTR)(scb->seg_buff + len), 
					scb->seg_len, (PTR)scb->seg_buff );
			}

			/*
			** Now send data from current segment.
			*/
			if ( seg_len >= char_size  &&  
			     RCB_AVAIL( *rcb_ptr ) >= char_size )
			{
			    len = min( seg_len, RCB_AVAIL( *rcb_ptr ) );
			    chrs = len / char_size;
			    len = chrs * char_size;

			    if ( ! ucs2 )
				jdbc_put_bytes( rcb_ptr, len, ptr );
			    else
				jdbc_put_ucs2( rcb_ptr, chrs, ptr );

			    ptr += len;
			    seg_len -= len;
			}
		    }

		    jdbc_msg_end( *rcb_ptr, FALSE );
		    jdbc_msg_begin( rcb_ptr, JDBC_MSG_DATA, 0 );
		}

		/*
		** Save any data left in the current segment.
		** The preceding loop makes sure there is room
		** in the buffer for the remainder of the
		** current segment.
		*/
		if ( seg_len )
		{
		    if ( ! scb->seg_buff )
		    {
			scb->seg_buff = (u_i1 *)MEreqmem( 0, scb->seg_max,
							  FALSE, NULL );
			if ( ! scb->seg_buff )
			{
			    if ( JDBC_global.trace_level >= 1 )
				TRdisplay( "%4d    JDBC alloc fail seg: %d\n",
					   ccb->id, scb->seg_max );
			    gcu_erlog( 0, JDBC_global.language, 
				       E_JD0108_NO_MEMORY, NULL, 0, NULL );
			    jdbc_gcc_abort( ccb, E_JD0108_NO_MEMORY );
			}
		    }

		    if ( JDBC_global.trace_level >= 5 )
			TRdisplay( "%4d    JDBC save segment: %d (total %d) \n",
				    ccb->id, seg_len, scb->seg_len + seg_len );
		    MEcopy( (PTR)ptr, seg_len, 
			    (PTR)(scb->seg_buff + scb->seg_len) );
		    scb->seg_len += seg_len;
		}

		/*
		** When the BLOB is complete, write the last
		** segment and end-of-segments indicator.
		*/
		if ( ! (scb->column.more_segments = more_segments) )
		{
		    /*
		    ** The processing loop above makes sure there
		    ** is room for the last segment.
		    */
		    if ( scb->seg_len )
		    {
			if ( JDBC_global.trace_level >= 5 )
			    TRdisplay( "%4d    JDBC send segment: %d \n",
					ccb->id, scb->seg_len );

			chrs = scb->seg_len / char_size;
			len = chrs * char_size;
			scb->seg_len = 0;

			jdbc_put_i2( rcb_ptr, chrs );

			if ( ! ucs2 )
			    jdbc_put_bytes( rcb_ptr, len, scb->seg_buff );
			else
			    jdbc_put_ucs2( rcb_ptr, chrs, scb->seg_buff );
		    }

		    if ( JDBC_global.trace_level >= 5 )
			TRdisplay( "%4d    JDBC send end-of-segments\n",
				    ccb->id );
		    jdbc_put_i2( rcb_ptr, 0 );
		}
	    }
	    break;

	default :
	    /* Should not happen since checked in send_desc() */
	    if ( JDBC_global.trace_level >= 1 )
		TRdisplay( "%4d    JDBC invalid datatype: %d\n",
				    ccb->id, desc[ col ].ds_dataType );
	    jdbc_gcc_abort( ccb, E_JD0112_UNSUPP_SQL_TYPE );
	    return;
	}
    }

    return;
}

