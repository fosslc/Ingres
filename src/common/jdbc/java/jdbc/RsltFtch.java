/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbc;

/*
** Name: RsltFtch.java
**
** Description:
**	Implements the EDBC JDBC ResultSet class RsltFtch
**	for handling result-sets whose data is fetched from
**	the JDBC server..
**
**  Classes
**
**	RsltFtch	Fetch result-set.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	 8-Sep-99 (gordy)
**	    Created new base class for class which interact with
**	    the JDBC server and extracted common data and methods.
**	    Synchronize entire request/response with JDBC server.
**	13-Sep-99 (gordy)
**	    Implemented error code support.
**	29-Sep-99 (gordy)
**	    Added class to support BLOBs as streams.  Implemented
**	    BLOB data support.  Use DbConn lock()/unlock() methods
**	    for synchronization.
**	11-Nov-99 (gordy)
**	    Extracted base functionality to EdbcRslt.java.
**	12-Nov-99 (gordy)
**	    Use configured date formatter.
**	15-Nov-99 (gordy)
**	    Added max row count and max column length.
**	18-Nov-99 (gordy)
**	    Made BLOB stream classes nested.
**	23-Nov-99 (gordy)
**	    Handle Ingres 'empty' dates.
**	 8-Dec-99 (gordy)
**	    Since col_cnt is only relevent to BLOBS and this result
**	    set class, made it a private member of this class.
**	13-Dec-99 (gordy)
**	    Added support for pre-fetching of rows.
**	 5-Jan-00 (gordy)
**	    Close result set at end-of-rows or an exception occurs.
**	25-Jan-00 (gordy)
**	    Added tracing.
**	04-Feb-00 (rajus01)
**	    Added getMetaData().
**	19-May-00 (gordy)
**	    Track closed status.  Use class methods for locking to
**	    isolate lock strategy and permit override by sub-classes.
**	    Close cursor when end-of-data is reached.
**	 4-Oct-00 (gordy)
**	    Standardized internal tracing.
**	27-Oct-00 (gordy)
**	    Removed locking (responsibility of sub-classes) and renamed
**	    to RsltFtch.
**	 3-Nov-00 (gordy)
**	    Added support for JDBC 2.0 extensions.
**	23-Jan-01 (gordy)
**	    Changed parameter type to EdbcStmt for backward compatibility.
**	31-Jan-01 (gordy)
**	    Don't throw protocol exceptions for invalid date formats.
**	    EdbcRslt now handles these as conversion errors.
**	28-Mar-01 (gordy)
**	    Combined functionality of close() into shut().  Renamed
**	    shut() to closeCursor() and created new shut().
**	 4-Apr-01 (gordy)
**	    Empty Date DataTruncation warnings moved to EdbcRslt.
**	18-Apr-01 (gordy)
**	    Use readBytes() method which reads length from input message.
**	10-May-01 (gordy)
**	    Support UCS2 transport format for character datatypes.
**	19-Jun-02 (gordy)
**	    BUG 108286 - Protocol error closing result-set with blob column
**	    which was not accessed with getXXX().  Changed the processing 
**	    order in shut() to perform the local class close operations 
**	    before calling the super class shut().
*/

import	java.math.BigDecimal;
import	java.sql.Types;
import	java.sql.SQLException;
import	ca.edbc.util.EdbcEx;
import	ca.edbc.io.DbConn;


/*
** Name: RsltFtch
**
** Description:
**	EDBC JDBC Java driver class which implements the
**	JDBC ResultSet interface for DBMS result data which
**	is fetched from the JDBC server.  While not abstract,
**	this class should be sub-classed to provide data
**	source specific support and connection locking.
**
**	This class supports both multi-row fetching (pre-fetch)
**	and partial row fetches (for BLOB support), though the
**	two are not expected to occur at the same time.  BLOBs 
**	result in partial row processing, and a BLOB is not 
**	available once processing passes beyond the BLOB, so
**	BLOBs and multi-row fetching should not be combined.
**
**	When a BLOB is read a flag is set indicating an active
**	BLOB data stream and the column processing is interrupted.
**	Column processing is resumed once the BLOB data stream
**	is closed.  Request for a column after the BLOB, the
**	next row, or closing the data set forces the BLOB stream
**	to be closed.  The database connection remains locked
**	during the entire lifetime of a BLOB data stream.
**
**	This class does not handle locking of the connection,
**	leaving it as a responsibility of sub-classes.  Methods
**	which may require locking support are load(), shut(),
**	flush() and resume().
**
**  Implemented Methods:
**
**	load		    Load data subset.
**	shut		    Close result-set and free resources.
**	resume		    Continue result processing after BLOB.
**
**  Overridden Methods:
**
**	readData	    Read data from server.
**
**  Protected Methods
**
**	closeCursor	    Close cursor.
**	flush		    Flush data-set.
**
**  Protected Data:
**
**	cursorClosed	    Cursor is closed.
**	blob_active	    Is current column an active BLOB stream.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	 8-Sep-99 (gordy)
**	    Created new base class for class which interact with
**	    the JDBC server and extracted common data and methods.
**	    Synchronize entire request/response with JDBC server.
**	29-Sep-99 (gordy)
**	    Implemented support for BLOB data and added support
**	    methods and fields.
**	11-Nov-99 (gordy)
**	    Extracted base functionality to EdbcRslt.
**	15-Nov-99 (gordy)
**	    Added row count and max row count.
**	 8-Dec-99 (gordy)
**	    Since col_cnt is only relevent to BLOBS and this result
**	    set class, made it a private member of this class.
**	13-Dec-99 (gordy)
**	    Added data_set, cur_row_cnt, current_row for pre-fetching.
**	 4-Feb-00 (gordy)
**	    Override the closeStrm() method to process columns whose
**	    streams are closed by the superclass such as when a stream
**	    is retrieved through the getString() method.
**	 4-May-00 (gordy)
**	    Extracted multi-row handling to base class.
**	19-May-00 (gordy)
**	    Added closed field for tracking status, and lock(), unlock()
**	    methods to permit sub-classes to control locking mode.
**	27-Oct-00 (gordy)
**	    Removed locking and cursor related fields/methods.  Renamed
**	    to RsltFtch.
**	28-Mar-01 (gordy)
**	    Renamed shut() to closeCursor() and closed to cursorClosed()
**	    Created new shut() method with functionality of close().
*/

class
RsltFtch
    extends RsltBlob
{

    protected boolean	    cursorClosed = false;
    protected boolean	    blob_active = false;


/*
** Name: RsltFtch
**
** Description:
**	Class constructor.
**
** Input:
**	stmt		Associated statement.
**	rsmd		Result set Meta-Data.
**	stmt_id		Statement ID.
**	preFetch	Pre-fetch row count.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	15-Nov-99 (gordy)
**	    Added max row count and max column length.
**	13-Dec-99 (gordy)
**	    Added fetch limit and multi-row data set.
**	 4-Oct-00 (gordy)
**	    Create unique ID for standardized internal tracing.
**	 3-Nov-00 (gordy)
**	    Parameters changed for JDBC 2.0 extensions.
**	23-Jan-01 (gordy)
**	    Changed parameter type to EdbcStmt for backward compatibility.
*/

public
RsltFtch
( 
    EdbcStmt	stmt, 
    EdbcRSMD	rsmd, 
    long	stmt_id,
    int		preFetch
)
    throws EdbcEx
{
    super( stmt, rsmd, preFetch );
    this.stmt_id = stmt_id;
    tr_id = "Ftch[" + inst_id + "]";
} // RsltFtch


/*
** Name: load
**
** Description:
**	Load columns (during partial row processing) at least 
**	upto the requested column.  Any preceding BLOB columns 
**	are abandoned.  The rest of the row is loaded, except if
**	a BLOB column exists at or beyond the requested column.
**
** Input:
**	index	Internal column index.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 5-May-00 (gordy)
**	    Created.
*/

protected void
load( int index )
    throws EdbcEx
{
    /*
    ** We are scanning forward on a partial row fetch,
    ** abandoning any intervening BLOBs.
    */
    while( index >= cur_col_cnt  &&  blob_active )  closeStrm();

    if ( ! blob_active  &&  cur_col_cnt > 0 )
    {
	/*
	** This should not happen.  Trace and patch.
	*/
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ".load(): partial row fetch but no BLOB active!" );
	cur_col_cnt = 0;
    }
    return;
}


/*
** Name: load
**
** Description:
**	Load a new data subset.  Any existing data is being abandoned,
**	including partial row fetches.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	 8-Sep-99 (gordy)
**	    Synchronize on DbConn.
**	29-Sep-99 (gordy)
**	    Use DbConn lock()/unlock() methods for synchronization.
**	 5-May-00 (gordy)
**	    Extracted multi-row handling to EdbcRslt and renamed load().
**	19-May-00 (gordy)
**	    Use class lock(), unlock() methods to isolate lock strategy.
**	    Check for closed state.  Close the cursor when end-of-data 
**	    is reached.
**	27-Oct-00 (gordy)
**	    Removed locking (sub-class responsibility).
**	28-Mar-01 (gordy)
**	    Renamed close() to shut() and shut() to closeCursor().
*/

protected void
load()
    throws EdbcEx
{
    if ( cursorClosed )
    {
	current_row = cur_row_cnt = cur_col_cnt = 0;
	return;
    }

    try { flush(); }	    // close any active BLOBs.
    catch( EdbcEx ex )
    {
	try { shut(); }  catch( EdbcEx ignore ) {}
	throw ex;
    }

    current_row = cur_row_cnt = cur_col_cnt = 0;
    
    try
    {
	dbc.begin( JDBC_MSG_CURSOR );
	dbc.write( JDBC_CUR_FETCH );
	dbc.write( JDBC_CUR_STMT_ID );
	dbc.write( (short)8 );
	dbc.write( (int)((stmt_id >> 32 ) & 0xffffffff) );
	dbc.write( (int)(stmt_id & 0xffffffff) );

	if ( data_set.length > 1 )
	{
	    dbc.write( JDBC_CUR_PRE_FETCH );
	    dbc.write( (short)4 );
	    dbc.write( (int)data_set.length );
	}

	dbc.done( true );
	readResults();
    }
    catch( EdbcEx ex )
    {
	try { shut(); }  catch( EdbcEx ignore ) {}
	throw ex;
    }

    /*
    ** Partial tuple receives are not permitted
    ** except for BLOBs which interrupt the 
    ** column processing until BLOB is closed.
    */
    if ( cur_col_cnt > 0  &&  ! blob_active )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( tr_id + "next: only received " + cur_col_cnt + 
			 " of " + rsmd.count + " columns" );
	try { shut(); }  catch( EdbcEx ignore ) {}
	throw EdbcEx.get( E_JD0002_PROTOCOL_ERR );
    }

    /*
    ** Close the cursor or tuple stream when end-of-data detected.
    */
    if ( resultEOD )  try { closeCursor(); }  catch( EdbcEx ignore ) {}

    return;
}


/*
** Name: closeCursor
**
** Description:
**	Close the cursor and free associated resources.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	 8-Sep-99 (gordy)
**	    Synchronize on DbConn.
**	29-Sep-99 (gordy)
**	    Use DbConn lock()/unlock() methods for synchronization.
**	04-Feb-00 (rajus01)
**	    Reset the counters after flush().
**	 8-Feb-00 (gordy)
**	    Check for connection closed and complete silently.
**	 5-May-00 (gordy)
**	    Extracted base functionality to EdbcRslt and renamed shut().
**	19-May-00 (gordy)
**	    Track closed status.  Use class lock(), unlock() methods to
**	    isolate lock strategy.
**	27-Oct-00 (gordy)
**	    Removed locking (sub-class responsibility).
**	28-Mar-01 (gordy)
**	    Renamed to closeCursor()
*/

protected void
closeCursor()
    throws EdbcEx
{
    synchronized( this )
    {
	if ( cursorClosed )  return;
	cursorClosed = true;
    }

    if ( dbc.isClosed() )  
    {
	blob_active = false;
	return;
    }

    if ( trace.enabled( 3 ) )  trace.write( tr_id + ": closing cursor" );
    try { flush(); }  catch( EdbcEx ignore ) {}

    try
    {
	dbc.begin( JDBC_MSG_CURSOR );
	dbc.write( JDBC_CUR_CLOSE );
	dbc.write( JDBC_CUR_STMT_ID );
	dbc.write( (short)8 );
	dbc.write( (int)((stmt_id >> 32 ) & 0xffffffff) );
	dbc.write( (int)(stmt_id & 0xffffffff) );
	dbc.done( true );
	readResults();
    }
    catch( EdbcEx ex )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ".shut(): error closing cursor" );
	    ex.trace( trace );
	}
	throw ex;
    }

    if ( trace.enabled( 3 ) )  trace.write( tr_id + ": cursor closed!" );
    return;
} // closeCursor


/*
** Name: shut
**
** Description:
**	Close the cursor and free associated resources.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if result-set was open, false if already closed.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	 5-May-00 (gordy)
**	    Extracted base functionality to EdbcRslt and renamed shut().
**	28-Mar-01 (gordy)
**	    Renamed original shut() to closeCursor().  This method now
**	    assumes functionality of close().  Since there is no super-
**	    class method for closeCursor(), make sure cursor is closed
**	    when result-set is closed.
**	19-Jul-01 (gordy)
**	    Reversed the order of calling closeCursor() and super.shut().
**	    The super class shut() resets important global result-set
**	    parameters which are used during closeCursor() to clean-up
**	    the current state of data processing.  
*/

boolean	// package access
shut()
    throws EdbcEx
{
    boolean closing;
    
    try { closeCursor(); }
    finally { closing = super.shut(); }
    
    return( closing );
} // shut


/*
** Name: flush
**
** Description:
**	Flush the input message stream if called while
**	query results not completely received.  The only
**	condition under which this will happen is when
**	a BLOB column interrupted result processing and
**	is still active (completed BLOB columns restart
**	result processing).
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
**	13-Dec-99 (gordy)
**	    Don't need to check column counters, they are
**	    handled elsewhere.
**	27-Oct-00 (gordy)
**	    Clear the BLOB flag if an exception is thrown.
*/

protected void
flush()
    throws EdbcEx
{
    /*
    ** The easiest way to flush the input message queue
    ** is close the current stream (flushes input until
    ** the end of the BLOB is reached) and continue
    ** column processing.  Column processing will be
    ** interrupted for each subsequent BLOB, so the loop
    ** continues until no BLOB is active.  Some additional
    ** overhead will occur loading columns results that 
    ** will never be used, but this handles a complex
    ** situation correctly without any additional code.
    */
    try { while( blob_active )  closeStrm(); }
    finally { blob_active = false; }
    return;
} // flush


/*
** Name: resume
**
** Description:
**	Continue processing query results following column
**	which interrupted processing.  Currently, only
**	BLOB columns interrupt result processing, so this
**	method should only be called by the super-class
**	when the BLOB stream has been completely processed.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
**	15-Nov-99 (gordy)
**	    Bump row count once entire row has been read.
**	13-Dec-99 (gordy)
**	    Complete row is now determined by column count
**	    which must be reset for start of new row.
**	25-Jan-00 (gordy)
**	    Added tracing.
**	19-May-00 (gordy)
**	    Use class lock(), unlock() methods to isolate lock strategy.
**	27-Oct-00 (gordy)
**	    Removed locking (sub-class responsibility).
*/

protected void
resume()
    throws EdbcEx
{
    blob_active = false;
    
    if ( cur_col_cnt >= rsmd.count )
    {
	/*
	** We have completed a BLOB which was the last
	** column in a row.  This now completes the row
	** so bump the row counter and reset the column
	** counter.
	*/
	cur_row_cnt++;
	cur_col_cnt = 0;
    }

    try
    {
	/*
	** If the current message has been completed, call 
	** readResults() to retrieve and process the next 
	** message.  Otherwise, call readData() to process 
	** the current message.  If readData() does not
	** interrupt processing (it processed the rest of
	** the message), call readResults() to process
	** additional messages.  In all cases, either the
	** end of the result set has been received, or
	** another BLOB has been activated.
	*/
	if ( ! dbc.moreData() )
	{
	    if ( trace.enabled( 5 ) )  
		trace.write( tr_id + ".resume: read next message" );
	    readResults();
	}
	else
	{
	    if ( trace.enabled( 5 ) )  
		trace.write( tr_id + ".resume: current message" );

	    if ( ! readData() )  
	    {
		if ( trace.enabled( 5 ) )  
		    trace.write( tr_id + ".resume: next message");
		readResults();
	    }
	}
    }
    catch( EdbcEx ex )
    {
	try { shut(); }  catch( EdbcEx ignore ) {}
	throw ex;
    }

    return;
} // resume


/*
** Name: readData
**
** Description:
**	Read data from server.  Overrides default method in JdbcObject.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    Interrupt reading results.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented BLOB data support.  Changed return type
**	    to permit BLOB stream to interrupt message processing.
**	12-Nov-99 (gordy)
**	    Use configured date formatter.
**	23-Nov-99 (gordy)
**	    Ingres allows for 'empty' dates which format to zero length
**	    strings.  If such a beast is received, create an 'epoch'
**	    timestamp and generate a data truncation warning.
**	13-Dec-99 (gordy)
**	    Read multiple rows if complete row received and there is
**	    additional space in the data set.
**	30-May-00 (gordy & rajus01)
**	    Fixed the behaviour of select date('today') query.
**	    (Bug #s 101677, 101678).
**	31-Jan-01 (gordy)
**	    Don't throw protocol exceptions for invalid date formats.
**	    EdbcRslt now handles these as conversion errors.
**	 4-Apr-01 (gordy)
**	    Can't create a DataTruncation warning for empty dates here
**	    because row pre-fetching causes warnings for all rows to be
**	    returned on the first row.  Also, a preceding BLOB will
**	    cause the warning to be generated at some unexpected point.
**	18-Apr-01 (gordy)
**	    Use readBytes() method which reads length from input message.
**	10-May-01 (gordy)
**	    The character datatypes (CHAR, VARCHAR, LONGVARCHAR) may now
**	    be sent as UCS2 character arrays in addition to the existing
**	    Ingres Character Set strings.  The DBMS datatype is used to
**	    distinguish the transport format.
*/

protected boolean
readData()
    throws EdbcEx
{
    while( cur_row_cnt < data_set.length )
    {
	for( int col = cur_col_cnt; col < rsmd.count; col++, cur_col_cnt++ )
	{
	    Object	data[];
	    short	len;

	    /*
	    ** The data set row arrays are not pre-allocated, 
	    ** but are allocated on first use.  This is to 
	    ** avoid allocating a large number of arrays only
	    ** to have a small number of rows returned.
	    */
	    if ( data_set[ cur_row_cnt ] == null )
		data_set[ cur_row_cnt ] = new Object[ rsmd.count ];
	    data = data_set[ cur_row_cnt ];

	    /*
	    ** If we have read all the data in the current message,
	    ** we return without requesting an interrupt to allow
	    ** the next data message to be read.
	    */
	    if ( ! dbc.moreData() )  return( false );

	    /*
	    ** Read the data indicator byte.  If 0, data is NULL
	    ** and indicator is followed by next column.
	    */
	    if ( dbc.readByte() == 0 )
	    {
		data[ col ] = null;
		continue;
	    }

	    if ( ! dbc.moreData() )  
	    {
		/*
		** Only a BLOB segment may be separated from its
		** data/null indicator.  BLOBs are handled below,
		** otherwise it is a protocol error.
		*/
		if ( rsmd.desc[ col ].sql_type != Types.LONGVARCHAR  &&
		     rsmd.desc[ col ].sql_type != Types.LONGVARBINARY )
		{
		    if ( trace.enabled( 1 ) )
			trace.write( tr_id + ": data not present following data indicator" );
		    throw EdbcEx.get( E_JD0002_PROTOCOL_ERR );
		}
	    }

	    switch( rsmd.desc[ col ].sql_type )
	    {
	    case Types.BIT : 
		data[ col ] = new Boolean( (dbc.readByte() == 0) ? false : true );
		break;

	    case Types.TINYINT : 
		data[ col ] = new Byte( dbc.readByte() );
		break;

	    case Types.SMALLINT : 
		data[ col ] = new Short( dbc.readShort() );
		break;

	    case Types.INTEGER : 
		data[ col ] = new Integer( dbc.readInt() );
		break;

	    case Types.REAL : 
		data[ col ] = new Float( dbc.readFloat() ); 
		break;

	    case Types.FLOAT :
	    case Types.DOUBLE :	
		data[ col ] = new Double(dbc.readDouble()); 
		break;

	    case Types.NUMERIC :
	    case Types.DECIMAL : 
		data[ col ] = new BigDecimal(dbc.readString());  
		break;

	    case Types.TIMESTAMP :
		data[ col ] = dbc.readString();
		break;

	    case Types.CHAR :
		if ( rsmd.desc[ col ].dbms_type != DBMS_TYPE_NCHAR )
		    data[ col ] = dbc.readString( rsmd.desc[ col ].length );
		else
		    data[ col ] = dbc.readUCS2( rsmd.desc[col].length / 2 );
		break;

	    case Types.VARCHAR :    
		if ( rsmd.desc[ col ].dbms_type != DBMS_TYPE_NVARCHAR )
		    data[ col ] = dbc.readString();
		else
		    data[ col ] = dbc.readUCS2();
		break;

	    case Types.LONGVARCHAR :
		if ( rsmd.desc[ col ].dbms_type != DBMS_TYPE_LONG_NCHAR )
		    data[ col ] = dbc.readAscii();
		else
		    data[ col ] = dbc.readUnicode();
		
		blob_active = true;
		cur_col_cnt++;
		return( true );

	    case Types.BINARY :
		data[ col ] = dbc.readBytes( rsmd.desc[ col ].length ); 
		break;

	    case Types.VARBINARY :  
		data[ col ] = dbc.readBytes();
		break;

	    case Types.LONGVARBINARY :
		data[ col ] = dbc.readBinary();
		blob_active = true;
		cur_col_cnt++;
		return( true );

	    default :
		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ": unexpected SQL type " + 
				 rsmd.desc[ col ].sql_type );
		throw EdbcEx.get( E_JD0002_PROTOCOL_ERR );
	    }
	}

	/*
	** We have completed a row.  Bump the row counter
	** and reset the column counter.
	*/
	cur_row_cnt++;
	cur_col_cnt = 0;
    }

    return( false );
} // readData

} // class RsltFtch

