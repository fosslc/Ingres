/*
** Copyright (c) 1999, 2010 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.dam;

/*
** Name: MsgOut.java
**
** Description:
**	Defines class providing capability to send messages to 
**	a Data Access Server.
**
**	The Data Access Messaging (DAM) protocol classes present 
**	a single unified interface, through inheritance, for access 
**	to a Data Access Server.  They are divided into separate 
**	classes to group related functionality for easier management.  
**	The order of inheritance is determined by the initial actions
**	required during initialization of the connection:
**
**	    MsgIo	Establish socket connection.
**	    MsgOut	Send TL Connection Request packet.
**	    MsgIn	Receive TL Connection Confirm packet.
**	    MsgConn	General initialization.
**
**  Classes:
**
**	MsgOut		Extends MsgIo.
**	ByteSegOS	Segmented byte OutputStream.
**	Ucs2SegWtr	Segmented UCS2 Writer.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	10-Sep-99 (gordy)
**	    Parameterized the initial TL connection data.
**	13-Sep-99 (gordy)
**	    Implemented error code support.
**	29-Sep-99 (gordy)
**	    Added Segmentor class to process BLOBs as streams.  
**	    Added methods (write(InputStream) and write(Reader)) 
**	    to process BLOBs.  Moved check() functionality to begin().
**	17-Nov-99 (gordy)
**	    Extracted output functionality from DbConn.java.
**	21-Apr-00 (gordy)
**	    Moved to package io.
**	12-Apr-01 (gordy)
**	    Pass tracing to exception.
**	10-May-01 (gordy)
**	    Added support for UCS2 datatypes.  Renamed segmentor class.
**	31-May-01 (gordy)
**	    Fixed UCS2 BLOB support.
**	 6-Sep-02 (gordy)
**	    Character encoding now encapsulated in CharSet class.
**	 1-Oct-02 (gordy)
**	    Moved to GCF dam package.  Renamed as DAM Message
**	    Layer implementation class.
**	31-Oct-02 (gordy)
**	    Renamed GCF error codes.
**	20-Dec-02 (gordy)
**	    Header ID protocol level dependent.  Communicate TL protocol
**	    level to rest of messaging system.
**	15-Jan-03 (gordy)
**	    Added MSG layer ID connection parameter.
**	25-Feb-03 (gordy)
**	    CharSet now supports character arrays.
**	 1-Dec-03 (gordy)
**	    Support SQL data object values.  Added UCS2 segmentor class.
**	15-Mar-04 (gordy)
**	    Support SQL BIGINT and Java long values.
**	19-Jun-06 (gordy)
**	    ANSI Date/Time data type support.
**	15-Nov-06 (gordy)
**	    Support LOB Locators.
**	26-Feb-07 (gordy)
**	    Support for buffered LOBs.
**	 5-Dec-07 (gordy)
**	    Adding support for multiple server targets.
**      19-Nov-08 (rajus01) SIR 121238
**          Replaced SqlEx references with SQLException or SqlExFactory
**          depending upon the usage of it. SqlEx becomes obsolete to support
**          JDBC 4.0 SQLException hierarchy.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	26-Oct-09 (gordy)
**	    Support SQL BOOLEAN and Java boolean values.
**	25-Mar-10 (gordy)
**	    Added support for batch processing.
*/

import	java.io.InputStream;
import	java.io.Reader;
import	java.io.OutputStream;
import	java.io.Writer;
import	java.io.IOException;
import	java.sql.SQLException;
import	com.ingres.gcf.util.GcfErr;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.util.IdMap;
import	com.ingres.gcf.util.Trace;
import	com.ingres.gcf.util.TraceLog;
import	com.ingres.gcf.util.ByteArray;
import	com.ingres.gcf.util.CharArray;
import	com.ingres.gcf.util.SqlData;
import	com.ingres.gcf.util.SqlNull;
import	com.ingres.gcf.util.SqlTinyInt;
import	com.ingres.gcf.util.SqlSmallInt;
import	com.ingres.gcf.util.SqlInt;
import	com.ingres.gcf.util.SqlBigInt;
import	com.ingres.gcf.util.SqlReal;
import	com.ingres.gcf.util.SqlDouble;
import	com.ingres.gcf.util.SqlDecimal;
import	com.ingres.gcf.util.SqlBool;
import	com.ingres.gcf.util.SqlDate;
import	com.ingres.gcf.util.SqlTime;
import	com.ingres.gcf.util.SqlTimestamp;
import	com.ingres.gcf.util.IngresDate;
import	com.ingres.gcf.util.SqlByte;
import	com.ingres.gcf.util.SqlVarByte;
import	com.ingres.gcf.util.SqlLongByte;
import	com.ingres.gcf.util.SqlLongByteCache;
import	com.ingres.gcf.util.SqlChar;
import	com.ingres.gcf.util.SqlNChar;
import	com.ingres.gcf.util.SqlVarChar;
import	com.ingres.gcf.util.SqlNVarChar;
import	com.ingres.gcf.util.SqlLongChar;
import	com.ingres.gcf.util.SqlLongCharCache;
import	com.ingres.gcf.util.SqlLongNChar;
import	com.ingres.gcf.util.SqlLongNCharCache;
import	com.ingres.gcf.util.SqlStream;
import	com.ingres.gcf.util.SqlLoc;


/*
** Name: MsgOut
**
** Description:
**	Class providing methods for sending messages to a Data
**	Access Server.  Provides methods for building messages
**	of the Message Layer of the Data Access Messaging protocol 
**	(DAM-ML).  Drives the Transport Layer (DAM-TL) using the 
**	buffered output class OutBuff to send TL  packets to the 
**	server.
**
**	When sending messages, the caller must provide multi-
**	threaded protection for each message.
**
**  Public Methods:
**
**	begin			Start a new output message.
**	write			Append data to output message.
**	writeUCS2		Append char data as UCS2 to output message.
**	done			Finish the current output message.
**
**  Protected Methods:
**
**	connect			Connect to target server.
**	sendCR			Send connection request.
**	disconnect		Shutdown the physical connection.
**	close			Shutdown the logical (TL) connection.
**	setBuffSize		Set the TL buffer size.
**	setIoProtoLvl		Set the I/O protocol level.
**
**  Private Data:
**
**	out			Output buffer.
**	segOS			Segmented OutputStream.
**	segWtr			Segmented Writer.
**	out_msg_id		Current output message ID.
**	out_hdr_pos		Message header buffer position.
**	out_hdr_len		Size of message header.
**
**  Private Methods:
**
**	writeSqlDataIndicator	Append the SQL data/NULL inidicator byte.
**	split			Send current output message and continue.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	10-Sep-99 (gordy)
**	    Parameterized the initial TL connection data.
**	29-Sep-99 (gordy)
**	    Added methods (write(InputStream) and write(Reader)) 
**	    to process BLOBs.  Moved check() functionality to begin().
**	17-Nov-99 (gordy)
**	    Extracted output functionality from DbConn.
**	10-May-01 (gordy)
**	    Added write() methods for byte and character arrays and
**	    writeUCS2() methods for UCS2 data.  Added character buffers,
**	    ca1 and char_buff, for write() methods.
**	 6-Sep-02 (gordy)
**	    Character encoding now encapsulated in CharSet class.  Added
**	    methods to return the encoded length of strings.
**	 1-Oct-02 (gordy)
**	    Renamed as DAM-ML messaging class.  Moved tl_proto and 
**	    char_set here from super-class.
**	20-Dec-02 (gordy)
**	    Communicate TL protocol level to rest of messaging system
**	    by adding setIoProtoLvl().  Moved tl_proto and char_set back
**	    to super-class to localize all internal data.
**	 1-Dec-03 (gordy)
**	    Added write methods for ByteArrays, CharArrays, and SQL 
**	    data objects and encapsulated UCS2 BLOB processing in a 
**	    UCS2 segmented writer.  Removed char_buff, string length 
**	    methods, and write methods obsoleted by SQL data object 
**	    support.
**	15-Mar-04 (gordy)
**	    Added write methods for long and SqlBigInt values.
**	19-Jun-06 (gordy)
**	    Added write methods for SqlDate, SqlTime, and SqlTimestamp.
**	15-Nov-06 (gordy)
**	    Added method to write LOB Locator values.  Added writeUCS2() 
**	    method for String.  Added write() and writeUCS2() methods
**	    for byte and character (segmented) streams.
**	26-Feb-07 (gordy)
**	    Added methods to write buffered LOBs.
**	 5-Dec-07 (gordy)
**	    Extract connection functionality from constructor to
**	    connect() and sendCR().
**	26-Oct-09 (gordy)
**	    Added write() method for boolean.
**	25-Mar-10 (gordy)
**	    Added done() method which takes explicit header flags.
*/

class
MsgOut
    extends MsgIo
    implements TlConst, MsgConst, GcfErr
{

    /*
    ** Output buffer and BLOB stream.
    */
    private OutBuff	out = null;
    private ByteSegOS	segOS = null;
    private Ucs2SegWtr	segWtr = null;

    /*
    ** Current message info
    */
    private byte	out_msg_id = 0;
    private int		out_hdr_pos = 0;
    private int		out_hdr_len = 0;


/*
** Name: MsgOut
**
** Description:
**	Class constructor.
**
** Input:
**	log		Trace log.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	 5-Dec-07 (gordy)
**	    Extracted connect() and sendCR() functionality.
*/

protected
MsgOut( TraceLog log )
    throws SQLException
{
    super( log );
    title = "MsgOut[" + connID() + "]";
} // MsgOut


/*
** Name: connect
**
** Description:
**	Connect to target server.  Initializes the output buffer.  
**
** Input:
**	host	Host name or address.
**	portID	Symbolic or numeric port ID.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	17-Nov-99 (gordy)
**	    Extracted output functionality from DbConn.
**	 5-Dec-07 (gordy)
**	    Extracted from constructor.
*/

protected void
connect( String host, String portID )
    throws SQLException
{
    super.connect( host, portID );

    try 
    {
	out = new OutBuff( socket.getOutputStream(), 
			   connID(), 1 << DAM_TL_PKT_MIN, trace.getTraceLog() );
    }
    catch( Exception ex )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( title + ": error creating output buffer: " + 
			 ex.getMessage() );
	disconnect();
	throw SqlExFactory.get( ERR_GC4001_CONNECT_ERR, ex );
    }

    return;
} // connect


/*
** Name: sendCR
**
** Description:
**	Sends the DAM-TL Connection Request packet including
**	the DAM-ML connection parameters.
**
** Input:
**	msg_cp		DAM-ML connection parameters.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	10-Sep-99 (gordy)
**	    Parameterized the initial TL connection data.
**	 1-Oct-02 (gordy)
**	    Renamed as DAM Message Layer.  Removed connection info 
**	    length parameter (use array length).  Added trace log 
**	    parameter.
**	15-Jan-03 (gordy)
**	    Added MSG layer ID connection parameter.
**	 5-Dec-07 (gordy)
**	    Extracted from constructor.
*/

protected void
sendCR( byte msg_cp[] )
    throws SQLException
{
    try
    {
	if ( trace.enabled( 2 ) )
	    trace.write( title + ": open TL Connection" );

	/*
	** Send the Connection Request message with our desired
	** protocol level, buffer size and DAM-ML info block.
	*/
	out.begin( DAM_TL_CR, 12 );
	out.write( DAM_TL_CP_PROTO );
	out.write( (byte)1 );
	out.write( DAM_TL_PROTO_DFLT );
	out.write( DAM_TL_CP_SIZE );
	out.write( (byte)1 );
	out.write( DAM_TL_PKT_MAX );
	out.write( DAM_TL_CP_MSG_ID );
	out.write( (byte)4 );
	out.write( DAM_TL_MSG_ID );

	if ( msg_cp != null  &&  msg_cp.length > 0 )
	{
	    out.write( DAM_TL_CP_MSG );

	    if ( msg_cp.length < 255 )
		out.write( (byte)msg_cp.length );
	    else
	    {
		out.write( (byte)255 );
		out.write( (short)msg_cp.length );
	    }
	    
	    out.write( msg_cp, 0, msg_cp.length );
	}

	out.flush();
    }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( title + ": error formatting/sending parameters" );
	disconnect();
	throw ex;
    }

    return;
} // sendCR


/*
** Name: disconnect
**
** Description:
**	Disconnect from server and free all I/O resources.
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
**	 7-Jun-99 (gordy)
**	    Created.
**	17-Nov-99 (gordy)
**	    Extracted output functionality from DbConn.
*/

protected void
disconnect()
{
    /*
    ** We don't set the output buffer references to null
    ** here so that we don't have to check it on each
    ** use.  I/O buffer functions will continue to work
    ** until a request results in a stream I/O request,
    ** in which case an exception will be thrown by the
    ** I/O buffer.
    **
    ** We must, however, test the reference for null
    ** since we may be called by the constructor with
    ** a null output buffer.
    */
    if ( out != null )
    {
	try { out.close(); }
	catch( Exception ignore ) {}
    }

    super.disconnect();
    return;
} // disconnect


/*
** Name: close
**
** Description:
**	Close the logical connection.
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
**	 7-Jun-99 (gordy)
**	    Created.
**	17-Nov-99 (gordy)
**	    Extracted output functionality from DbConn.
*/

protected void
close()
{
    if ( trace.enabled( 2 ) )
	trace.write( title + ": close TL connection" );

    try
    {
	out.clear();    // Clear partial messages from buffer.
	out.begin( DAM_TL_DR, 0 );
	out.flush();
    }
    catch( Exception ignore ) {}

    return;
} // close


/*
** Name: setBuffSize
**
** Description:
**	Set the I/O buffer size.
**
** Input:
**	size	    Size of I/O buffer.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	17-Nov-99 (gordy)
**	    Created.
*/

protected void
setBuffSize( int size )
{
    out.setBuffSize( size );
    return;
} // setBuffSize


/*
** Name: setIoProtoLvl
**
** Description:
**	Set the I/O protocol level.
**
** Input:
**	proto_lvl	DAM-TL protocol level.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	20-Dec-02 (gordy)
**	    Created.
*/

protected void
setIoProtoLvl( byte proto_lvl )
{
    out.setProtoLvl( proto_lvl );
    return;
} // setIoProtoLvl


/*
** Name: begin
**
** Description:
**	Begin a new message.  Builds a message header for
**	an initially empty message.  The caller should lock
**	this DbConn object from the time this method is
**	called until the request is complete.
**
** Input:
**	msg_id	    Message ID.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Replaced check() with inline test.
**	20-Dec-02 (gordy)
**	    Header ID protocol level dependent.
*/

public void
begin( byte msg_id )
    throws SQLException
{
    if ( isClosed() )  throw SqlExFactory.get( ERR_GC4004_CONNECTION_CLOSED );
    if ( out_hdr_pos > 0 )  done( false );	/* Complete current message */

    if ( trace.enabled( 3 ) )
	trace.write( title + ": begin message " + IdMap.map(msg_id, msgMap) );

    /*
    ** Reserve space in output buffer for header.
    ** Note that we don't do message concatenation
    ** at this level.  We rely on messages being
    ** concatenated by the output buffer if not
    ** explicitly flushed.
    */
    try 
    { 
	out.begin( DAM_TL_DT, 8 );	// Begin new message
	out_msg_id = msg_id;
	out_hdr_pos = out.position();
    
	out.write( (msg_proto_lvl >= MSG_PROTO_3) 
		    ? MSG_HDR_ID_2 : MSG_HDR_ID_1 );	// Eyecatcher
	out.write( (short)0 );				// Data length
	out.write( msg_id );				// Message ID
	out.write( MSG_HDR_EOD );			// Flags

	out_hdr_len = out.position() - out_hdr_pos;
    }
    catch( SQLException ex )
    {
	disconnect();
	throw ex;
    }

    return;
} // begin

/*
** Name: write
**
** Description:
**	Append a byte value to current output message.  If buffer
**	overflow would occur, the message is split.
**
** Input:
**	value	    The byte value to be appended.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
*/

public void
write( byte value )
    throws SQLException
{
    if ( out.avail() < 1 )  split();
    out.write( value );
    return;
} // write

/*
** Name: write
**
** Description:
**	Append a short value to current output message.  If buffer
**	overflow would occur, the message is split.
**
** Input:
**	value	    The short value to be appended.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
*/

public void
write( short value )
    throws SQLException
{
    if ( out.avail() < 2 )  split();
    out.write( value );
    return;
} // write

/*
** Name: write
**
** Description:
**	Append an integer value to current output message.  If buffer
**	overflow would occur, the message is split.
**
** Input:
**	value	    The integer value to be appended.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
*/

public void
write( int value )
    throws SQLException
{
    if ( out.avail() < 4 )  split();
    out.write( value );
    return;
} // write

/*
** Name: write
**
** Description:
**	Append a long value to current output message.  If buffer
**	overflow would occur, the message is split.
**
** Input:
**	value	    The long value to be appended.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	15-Mar-04 (gordy)
**	    Created.
*/

public void
write( long value )
    throws SQLException
{
    if ( out.avail() < 8 )  split();
    out.write( value );
    return;
} // write

/*
** Name: write
**
** Description:
**	Append a float value to current output message.  If buffer
**	overflow would occur, the message is split.
**
** Input:
**	value	    The float value to be appended.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
*/

public void
write( float value )
    throws SQLException
{
    if ( out.avail() < DAM_TL_F4_LEN )  split();
    out.write( value );
    return;
} // write

/*
** Name: write
**
** Description:
**	Append a double value to current output message.  If buffer
**	overflow would occur, the message is split.
**
** Input:
**	value	    The double value to be appended.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
*/

public void
write( double value )
    throws SQLException
{
    if ( out.avail() < DAM_TL_F8_LEN )  split();
    out.write( value );
    return;
} // write


/*
** Name: write
**
** Description:
**	Append a byte array to current output message.  If buffer
**	overflow would occur, the message (and array) is split.
**	The byte array is preceded by a two byte length indicator.
**
** Input:
**	value	    The byte array to be appended.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	22-Sep-99 (gordy)
**	    Send entire array, preceded by the length.
*/

public void
write( byte value[] )
    throws SQLException
{
    write( (short)value.length );
    write( value, 0, value.length );
    return;
} // write


/*
** Name: write
**
** Description:
**	Append a byte sub-array to current output message.  If buffer
**	overflow would occur, the message (and array) is split.
**
** Input:
**	value	    The byte array containing sub-array to be appended.
**	offset	    Start of sub-array.
**	length	    Length of sub-array.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	10-May-01 (gordy)
**	    Created.
*/

public void
write( byte value[], int offset, int length )
    throws SQLException
{
    for( int end = offset + length; offset < end; )
    {
	if ( out.avail() <= 0 )  split();
	length = Math.min( out.avail(), end - offset );
	out.write( value, offset, length );
	offset += length;
    }

    return;
} // write


/*
** Name: write
**
** Description:
**	Append a ByteArray to current output message.  If buffer
**	overflow would occur, the message (and array) is split.
**
** Input:
**	value	    The ByteArray to be appended.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
write( ByteArray bytes )
    throws SQLException
{
    int end = bytes.length();
    
    for( int offset = 0; offset < end;  )
    {
	if ( out.avail() <= 0 )  split();
	int len = Math.min( out.avail(), end - offset );
	len = out.write( bytes, offset, len );
	offset += len;
    }

    return;
} // write


/*
** Name: write
**
** Description:
**	Append a string value to current output message.  If buffer
**	overflow would occur, the message (and string) is split.
**	The string is preceded by a two byte length indicator.
**
** Input:
**	value	    The string value to be appended.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	22-Sep-99 (gordy)
**	    Added character set/encoding.
**	 6-Sep-02 (gordy)
**	    Character encoding now encapsulated in CharSet class.
*/

public void
write( String value )
    throws SQLException
{
    byte    ba[];

    try { ba = char_set.getBytes( value ); }
    catch( Exception ex )	// Should not happen!
    { throw SqlExFactory.get( ERR_GC401E_CHAR_ENCODE ); }

    write( ba );
    return;
} // write


/*
** Name: write
**
** Description:
**	Append a segmented byte stream to current output message.
**	If buffer overflow would occur, the message is split at
**	a segment boundary.
**
** Input:
**	is	Byte stream.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public void
write( InputStream is )
    throws SQLException
{
    if ( segOS == null )  segOS = new ByteSegOS();
    segOS.begin( out_msg_id );			// begin a new BLOB.
    try { SqlStream.copyIs2Os( is, segOS ); }	// Write stream to output.
    finally { segOS.end(); }			// End-of-segments.
    return;
} // write


/*
** Name: write
**
** Description:
**	Append a segmented character stream to current output message.
**	Character stream is converted from UCS2 to the host character-
**	set.  If buffer overflow would occur, the message is split at
**	a segment boundary.
**
** Input:
**	rdr	Character Stream.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public void
write( Reader rdr )
    throws SQLException
{
    Writer wtr;

    if ( segOS == null )  segOS = new ByteSegOS();
    try { wtr = char_set.getOSW( segOS ); }
    catch( Exception ex )	// Should not happen!
    { throw SqlExFactory.get( ERR_GC401E_CHAR_ENCODE ); }

    segOS.begin( out_msg_id );			// begin a new BLOB.
    try { SqlStream.copyRdr2Wtr( rdr, wtr ); }	// Write stream to output.
    finally { segOS.end(); }			// End-of-segments.
    return;
} // write


/*
** Name: writeUCS2
**
** Description:
**	Append a character sub-array to current output message.  
**	If buffer overflow would occur, the message (and array) 
**	is split.
**
** Input:
**	value	    The character array with sub-array to be appended.
**	offset	    Start of sub-array.
**	length	    Length of sub-array.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	10-May-01 (gordy)
**	    Created.
*/

public void
writeUCS2( char value[], int offset, int length )
    throws SQLException
{
    for( int end = offset + length; offset < end; offset++ )
	write( (short)value[ offset ] );
    return;
} // writeUCS2


/*
** Name: writeUCS2
**
** Description:
**	Append a character sub-array to current output message.  
**	If buffer overflow would occur, the message (and array) 
**	is split.
**
** Input:
**	value	    The character array with sub-array to be appended.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
writeUCS2( CharArray array )
    throws SQLException
{
    int end = array.length();
    
    for( int position = 0; position < end; position++ )
	write( (short)array.get( position ) );
    return;
} // writeUCS2


/*
** Name: writeUCS2
**
** Description:
**	Append a string value to current output message.  If buffer 
**	overflow would occur, the message (and string ) is split.  
**	The string is preceded by a two byte length indicator.
**
** Input:
**	value	    The string value to be appended.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public void
writeUCS2( String value )
    throws SQLException
{
    short len = (short)value.length();

    write( len );
    for( int idx = 0; idx < len; idx++ )  write( (short)value.charAt( idx ) );

    return;
} // writeUCS2


/*
** Name: writeUCS2
**
** Description:
**	Append a segmented UCS2 character stream to current output 
**	message.  If buffer overflow would occur, the message is 
**	split at a segment boundary.
**
** Input:
**	rdr	Character Stream.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public void
writeUCS2( Reader rdr )
    throws SQLException
{
    if ( segWtr == null )  segWtr = new Ucs2SegWtr();
    segWtr.begin( out_msg_id );			// begin a new BLOB.
    try { SqlStream.copyRdr2Wtr(rdr, segWtr); }	// Write stream to output.
    finally { segWtr.end(); }			// End-of-segments.
    return;
} // writeUCS2


/*
** Name: write
**
** Description:
**	Writes a SqlNull data value to the current input message.
**
**	A SqlNull data value is composed of simply a data indicator
**	byte.
**
** Input:
**	value	SQL data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
write( SqlNull value )
    throws SQLException
{
    if ( writeSqlDataIndicator( value ) )	// Write data indicator byte.
	throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );	// Should be NULL.
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a SqlTinyInt data value to the current input message.
**
**	A SqlTinyInt data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a one byte integer.
**
** Input:
**	value	SQL data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
write( SqlTinyInt value )
    throws SQLException
{
    if ( writeSqlDataIndicator( value ) )  write( value.get() );
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a SqlSmallInt data value to the current input message.
**
**	A SqlSmallInt data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a two byte integer.
**
** Input:
**	value	SQL data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
write( SqlSmallInt value )
    throws SQLException
{
    if ( writeSqlDataIndicator( value ) )  write( value.get() );
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a SqlInt data value to the current input message.
**
**	A SqlInt data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a four byte integer.
**
** Input:
**	value	SQL data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
write( SqlInt value )
    throws SQLException
{
    if ( writeSqlDataIndicator( value ) )  write( value.get() );
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a SqlBigInt data value to the current input message.
**
**	A SqlBigInt data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by an eight byte integer.
**
** Input:
**	value	SQL data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	15-Mar-04 (gordy)
**	    Created.
*/

public void
write( SqlBigInt value )
    throws SQLException
{
    if ( writeSqlDataIndicator( value ) )  write( value.get() );
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a SqlReal data value to the current input message.
**
**	A SqlReal data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a four byte floating
**	point value.
**
** Input:
**	value	SQL data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
write( SqlReal value )
    throws SQLException
{
    if ( writeSqlDataIndicator( value ) )  write( value.get() );
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a SqlDouble data value to the current input message.
**
**	A SqlDouble data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a eight byte floating
**	point value.
**
** Input:
**	value	SQL data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
write( SqlDouble value )
    throws SQLException
{
    if ( writeSqlDataIndicator( value ) )  write( value.get() );
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a SqlDecimal data value to the current input message.
**
**	A SqlDecimal data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a string which is
**	composed of a two byte integer indicating the byte length
**	of the string followed by a character byte array of the
**	indicated length.
**
** Input:
**	value	SQL data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
write( SqlDecimal value )
    throws SQLException
{
    if ( writeSqlDataIndicator( value ) )  write( value.get() );
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a SqlBool data value to the current input message.
**
**	A SqlBool data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a one byte integer.
**
** Input:
**	value	SQL data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	26-Oct-09 (gordy)
**	    Created.
*/

public void
write( SqlBool value )
    throws SQLException
{
    if ( writeSqlDataIndicator( value ) )  write((byte)(value.get() ? 1 : 0));
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a SqlDate data value to the current input message.
**
**	A SqlDate data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a string which is
**	composed of a two byte integer indicating the byte length
**	of the string followed by a character byte array of the
**	indicated length.
**
** Input:
**	value	SQL data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	19-Jun-06 (gordy)
**	    Created.
*/

public void
write( SqlDate value )
    throws SQLException
{
    if ( writeSqlDataIndicator( value ) )  write( value.get() );
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a SqlTime data value to the current input message.
**
**	A SqlTime data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a string which is
**	composed of a two byte integer indicating the byte length
**	of the string followed by a character byte array of the
**	indicated length.
**
** Input:
**	value	SQL data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	19-Jun-06 (gordy)
**	    Created.
*/

public void
write( SqlTime value )
    throws SQLException
{
    if ( writeSqlDataIndicator( value ) )  write( value.get() );
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a SqlTimestamp data value to the current input message.
**
**	A SqlTimestamp data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a string which is
**	composed of a two byte integer indicating the byte length
**	of the string followed by a character byte array of the
**	indicated length.
**
** Input:
**	value	SQL data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	19-Jun-06 (gordy)
**	    Created.
*/

public void
write( SqlTimestamp value )
    throws SQLException
{
    if ( writeSqlDataIndicator( value ) )  write( value.get() );
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a IngresDate data value to the current input message.
**
**	A IngresDate data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a string which is
**	composed of a two byte integer indicating the byte length
**	of the string followed by a character byte array of the
**	indicated length.
**
** Input:
**	value	SQL data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
write( IngresDate value )
    throws SQLException
{
    if ( writeSqlDataIndicator( value ) )  write( value.get() );
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a SqlByte data value to the current input message.
**	Uses the ByteArray interface to send the SqlByte data value.
**
**	A SqlByte data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a binary byte array
**	whose length is detemined by the length of the SqlByte data
**	value.
**
** Input:
**	value	SQL data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
write( SqlByte value )
    throws SQLException
{
    if ( writeSqlDataIndicator( value ) )  write( (ByteArray)value );
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a SqlVarByte data value to the current input message.
**	Uses the ByteArray interface to send the SqlVarByte data value.
**
**	A SqlVarByte data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a two byte integer length
**	which is followed by a binary byte array of the indicated length.
**
** Input:
**	value	SQL data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
write( SqlVarByte value )
    throws SQLException
{
    if ( writeSqlDataIndicator( value ) )	// Data indicator byte.
    {
	write( (short)value.length() );		// Array length.
	write( (ByteArray)value );		// Data array
    }
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a SqlChar data value to the current input message.
**	Uses the ByteArray interface to send the SqlChar data value.
**
**	A SqlChar data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a character byte array
**	whose length is detemined by the length of the SqlChar data 
**	value.
**
** Input:
**	value	SQL data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
write( SqlChar value )
    throws SQLException
{
    if ( writeSqlDataIndicator( value ) )  write( (ByteArray)value );
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a SqlVarChar data value to the current input message.
**	Uses the ByteArray interface to send the SqlVarChar data value.
**
**	A SqlVarChar data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a two byte integer length
**	which is followed by a character byte array of the indicated length.
**
** Input:
**	value	SQL data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
write( SqlVarChar value )
    throws SQLException
{
    if ( writeSqlDataIndicator( value ) )	// Data indicator byte.
    {
	write( (short)value.length() );		// Array length.
	write( (ByteArray)value );		// Data array
    }
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a SqlNChar data value to the current input message.
**	Uses the CharArray interface to send the SqlNChar data value.
**
**	A SqlNChar data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a UCS2 (two byte
**	integer) array whose length is detemined by the length of 
**	the SqlNChar data value.
**
** Input:
**	value	SQL data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
write( SqlNChar value )
    throws SQLException
{
    if ( writeSqlDataIndicator( value ) )  writeUCS2( (CharArray)value );
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a SqlNVarChar data value to the current input message.
**	Uses the CharArray interface to send the SqlNVarChar data value.
**
**	A SqlNVarChar data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a two byte integer length
**	which is followed by a UCS2 (two byte integer) array of the 
**	indicated length.
**
** Input:
**	value	SQL data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
write( SqlNVarChar value )
    throws SQLException
{
    if ( writeSqlDataIndicator( value ) )	// Data indicator byte.
    {
	write( (short)value.length() );		// Array length.
	writeUCS2( (CharArray)value );		// Data array
    }
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a SqlLoc data value to the current input message.
**
**	A locator data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a four byte integer.
**
** Input:
**	value	SQL data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public void
write( SqlLoc value )
    throws SQLException
{
    if ( writeSqlDataIndicator( value ) )  write( value.get() );
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a SqlLongByte data value to the current input message.
**	The binary stream is retrieved and segmented for output by 
**	an instance of the local inner class ByteSegOS.
**
**	A SqlLongByte data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by zero or more segments
**	which are then followed by an end-of-segments indicator.  Each
**	segment is composed of a two byte integer length (non-zero) 
**	followed by a binary byte array of the indicated length.  The
**	end-of-segments indicator is a two byte integer with value
**	zero.
**
** Input:
**	value	SQL data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
write( SqlLongByte value )
    throws SQLException
{
    if ( writeSqlDataIndicator( value ) )	// Data indicator byte.
    {
	if ( segOS == null )  segOS = new ByteSegOS();
	segOS.begin( out_msg_id );		// begin a new BLOB.
	try { value.get( segOS ); }		// Write stream to output.
	finally { segOS.end(); }		// End-of-segments.
    }
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a SqlLongByteCache data value to the current input 
**	message.  A binary stream is retrieved and segmented for 
**	output by an instance of the local inner class ByteSegOS.
**
**	A SqlLongByte data value is composed of a data indicator 
**	byte which, if not NULL, may be followed by zero or more 
**	segments which are then followed by an end-of-segments 
**	indicator.  Each segment is composed of a two byte integer 
**	length (non-zero) followed by a binary byte array of the 
**	indicated length.  The end-of-segments indicator is a two 
**	byte integer with value zero.
**
** Input:
**	value	SQL data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	26-Feb-07 (gordy)
**	    Created.
*/

public void
write( SqlLongByteCache value )
    throws SQLException
{
    if ( writeSqlDataIndicator( value ) )	// Data indicator byte.
    {
	if ( segOS == null )  segOS = new ByteSegOS();
	segOS.begin( out_msg_id );		// begin a new BLOB.
	try { value.get( segOS ); }		// Write stream to output.
	finally { segOS.end(); }		// End-of-segments.
    }
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a SqlLongChar data value to the current input message.
**	The byte stream is retrieved and segmented for output by an
**	instance of the local inner class ByteSegOS.
**
**	A SqlLongChar data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by zero or more segments
**	which are then followed by an end-of-segments indicator.  Each
**	segment is composed of a two byte integer length (non-zero) 
**	followed by a character byte array of the indicated length.  
**	The end-of-segments indicator is a two byte integer with value
**	zero.
**
** Input:
**	value	SQL data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
write( SqlLongChar value )
    throws SQLException
{
    if ( writeSqlDataIndicator( value ) )	// Data indicator byte.
    {
	if ( segOS == null )  segOS = new ByteSegOS();
	segOS.begin( out_msg_id );		// begin a new BLOB.
	try { value.get( segOS ); }		// Write stream to output.
	finally { segOS.end(); }		// End-of-segments.
    }
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a SqlLongCharCache data value to the current input message.
**	The byte stream is retrieved and segmented for output by an
**	instance of the local inner class ByteSegOS.
**
**	A SqlLongChar data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by zero or more segments
**	which are then followed by an end-of-segments indicator.  Each
**	segment is composed of a two byte integer length (non-zero) 
**	followed by a character byte array of the indicated length.  
**	The end-of-segments indicator is a two byte integer with value
**	zero.
**
** Input:
**	value	SQL data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	26-Feb-07 (gordy)
**	    Created.
*/

public void
write( SqlLongCharCache value )
    throws SQLException
{
    if ( writeSqlDataIndicator( value ) )	// Data indicator byte.
    {
	if ( segOS == null )  segOS = new ByteSegOS();
	segOS.begin( out_msg_id );		// begin a new BLOB.
	try { value.get( segOS ); }		// Write stream to output.
	finally { segOS.end(); }		// End-of-segments.
    }
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a SqlLongNChar data value to the current input message.
**	The character stream is retrieved and segmented for output by
**	an instance of the local inner class Ucs2SegWtr.
**
**	A SqlLongNChar data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by zero or more segments
**	which are then followed by an end-of-segments indicator.  Each
**	segment is composed of a two byte integer length (non-zero) 
**	followed by a UCS2 (two byte integer) array of the indicated 
**	length.  The end-of-segments indicator is a two byte integer 
**	with value zero.
**
** Input:
**	value	SQL data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
write( SqlLongNChar value )
    throws SQLException
{
    if ( writeSqlDataIndicator( value ) )	// Data indicator byte.
    {
	if ( segWtr == null )  segWtr = new Ucs2SegWtr();
	segWtr.begin( out_msg_id );		// begin a new BLOB.
	try { value.get( segWtr ); }		// Write stream to output.
	finally { segWtr.end(); }		// End-of-segments.
    }
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a SqlLongNCharCache data value to the current input message.
**	The character stream is retrieved and segmented for output by
**	an instance of the local inner class Ucs2SegWtr.
**
**	A SqlLongNChar data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by zero or more segments
**	which are then followed by an end-of-segments indicator.  Each
**	segment is composed of a two byte integer length (non-zero) 
**	followed by a UCS2 (two byte integer) array of the indicated 
**	length.  The end-of-segments indicator is a two byte integer 
**	with value zero.
**
** Input:
**	value	SQL data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	26-Feb-07 (gordy)
**	    Created.
*/

public void
write( SqlLongNCharCache value )
    throws SQLException
{
    if ( writeSqlDataIndicator( value ) )	// Data indicator byte.
    {
	if ( segWtr == null )  segWtr = new Ucs2SegWtr();
	segWtr.begin( out_msg_id );		// begin a new BLOB.
	try { value.get( segWtr ); }		// Write stream to output.
	finally { segWtr.end(); }		// End-of-segments.
    }
    return;
} // write


/*
** Name: writeSqlDataIndicator
**
** Description:
**	Writes a single byte (the SQL data/NULL indicator)
**	to the current message and returns True if the data 
**	value is not-NULL.
**
** Input:
**	value		SQL data value.
**
** Output:
**	None.
**
** Returns:
**	boolean		True if data value is not-NULL.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

private boolean
writeSqlDataIndicator( SqlData data )
    throws SQLException
{
    boolean isNull = data.isNull();
    write( (byte)(isNull ? 0 : 1) );
    return( ! isNull );
} // writeSqlNullIndicator


/*
** Name: done
**
** Description:
**	Finish the current message and optionally mark message
**	as end-of-group and send it to the server.
**
** Input:
**	flush	    True if message should be sent to server.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	25-Mar-10 (gordy)
**	    Now a cover for done( byte ).
*/

public void
done( boolean flush )
    throws SQLException
{  
    done( flush ? MSG_HDR_EOG : 0 ); 
    return;
} // done


/*
** Name: done
**
** Description:
**	Finish the current message and set the message header flags
**	(EOD is automatically added to flags).
**
** Input:
**	flags	Message header flags.
**
** Output:
**	None.
**
** Returns;
**	void
**
** History:
**	25-Mar-10 (gordy)
**	    Extracted from done( boolean ).
*/

public void
done( byte flags )
    throws SQLException
{
    flags |= MSG_HDR_EOD;	// Force EOD since message is done.

    if ( out_hdr_pos > 0 )	// Update the message header.
    {
	short length = (short)( out.position() - 
				(out_hdr_pos + out_hdr_len) );

	if ( trace.enabled( 2 ) )  
	    trace.write( title + ": sending message " + 
			 IdMap.map( out_msg_id, msgMap ) + 
			 " length " + length + 
			 ((flags & MSG_HDR_EOD) == 0 ? "" : " EOD") + 
			 ((flags & MSG_HDR_EOB) == 0 ? "" : " EOB") + 
			 ((flags & MSG_HDR_EOG) == 0 ? "" : " EOG") );
	
	out.write( out_hdr_pos + 4, length );
	out.write( out_hdr_pos + 7, flags );
    }

    if ( (flags & MSG_HDR_EOG) != 0 )
	try { out.flush(); }
	catch( SQLException ex )
	{
	    disconnect();
	    throw ex;
	}

    out_msg_id = -1;
    out_hdr_pos = 0;
    out_hdr_len = 0;

    return;
} // done

/*
** Name: split
**
** Description:
**	The current output message is sent to the server and 
**	a message continuation is initialized.
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
**	 7-Jun-99 (gordy)
**	    Created.
**	20-Dec-02 (gordy)
**	    Header ID protocol level dependent.
*/

private void
split()
    throws SQLException
{
    if ( out_hdr_pos > 0 )	
    {
	short length = (short)(out.position() - (out_hdr_pos + out_hdr_len));

	if ( trace.enabled( 3 ) )  
	    trace.write( title + ": split message " + 
			 IdMap.map(out_msg_id, msgMap) + " length " + length );

	// Update the message header.
	try
	{
	    out.write( out_hdr_pos + 4, length );
	    out.write( out_hdr_pos + 7, (byte)0 );
	    out.flush();

	    out.begin( DAM_TL_DT, 8 );		// Begin continued message
	    out_hdr_pos = out.position();
	    out.write( (msg_proto_lvl >= MSG_PROTO_3) 
			? MSG_HDR_ID_2 : MSG_HDR_ID_1);	// Eyecatcher
	    out.write( (short)0 );			// Data length
	    out.write( out_msg_id );			// Message ID
	    out.write( MSG_HDR_EOD );			// Flags
	    out_hdr_len = out.position() - out_hdr_pos;
	}
	catch( SQLException ex )
	{
	    disconnect();
	    throw ex;
	}
    }

    return;
} // split


/*
** Name: ByteSegOS
**
** Description:
**	Class for converting byte stream into segmented output stream.
**
**	This class is designed for repeated usage.  The begin() and
**	end() methods provide the means for a single class instance
**	to process multiple serial BLOBs on the same connection.  Note
**	that the close method does not actually close the OS object,
**	but simply ends the current BLOB output stream (same as end()).
**
**  Public Methods
**
**	begin		Initialize processing of a BLOB.
**	write		Write bytes in BLOB stream.
**	close		Close current BLOB stream.
**	end		Complete processing of a BLOB.
**
**  Private Data
**
**	title		Class title for tracing.
**	msg_id		Message ID of message containing the BLOB.
**	li_pos		Position of segment length indicator in output buffer.
**	ba		Temporary storage for writing single byte.
**
**  Private Methods:
**
**	startSegment	Start a new data segment.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
**	17-Nov-99 (gordy)
**	    Made class nested (inner).
**	10-May-01 (gordy)
**	    Class renamed to match changes in DbConnIn.  Made class 
**	    static and added msg_out for access to DB connection.
**	 1-Dec-03 (gordy)
**	    Changed back from static class and removed associated
**	    members msg_out, out, and trace.  Segment length now 
**	    calculated: removed seg_len and added startSegment().
*/

private class
ByteSegOS
    extends OutputStream
{

    private String	title;			// Class title for tracing.

    private byte	msg_id = -1;		// Current message ID.
    private int		li_pos = -1;		// Position of length indicator.

    private byte	ba[] = new byte[ 1 ];	// Temp storage.


/*
** Name: ByteSegOS
**
** Description:
**	Class constructor.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
**	17-Nov-99 (gordy)
**	    Removed dbc parameter when class nested.
**	10-May-01 (gordy)
**	    Added msg_out parameter when class made static.
**	 1-Dec-03 (gordy)
**	    Removed parameters when changed class to non-static.
*/

public
ByteSegOS()
{
    title = "ByteSegOS[" + MsgOut.this.connID() + "]";
} // BytesSegOS


/*
** Name: begin
**
** Description:
**	Initialize the segmentor for a new series of BLOB segments.
**	The current message ID must be provided since segmentation 
**	of the BLOB may require the current message to be completed 
**	and a new message of the same type to be started.
**
** Input:
**	msg_id	    ID of current message.
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
**	 1-Dec-03 (gordy)
**	    Use startSegment() to initialize segment.
*/

public void
begin( byte msg_id )
    throws SQLException
{
    this.msg_id = msg_id;
    li_pos = -1;
    
    if ( trace.enabled( 3 ) )  trace.write( title + ": start of BLOB" );
    startSegment();
    return;
}


/*
** Name: write
**
** Description:
**	Write a single byte to the segment stream.
**
** Input:
**	b	Byte to be output.
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
*/

public void
write( int b )
    throws IOException
{
    ba[ 0 ] = (byte)b;
    write( ba, 0, 1 );
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a byte array to the segment stream.
**
** Input:
**	bytes	Byte array to be output.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
write( byte bytes[] )
    throws IOException
{
    write( bytes, 0, bytes.length );
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a byte sub-array to the segment stream.  The remainder
**	of the current message buffer is used to build a segment.
**	When the buffer is filled, the current message is completed
**	and a new message of the same type is started.
**
** Input:
**	bytes	Byte array.
**	offset	Starting byte to be output.
**	length	Number of bytes to be output.
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
**	12-Apr-01 (gordy)
**	    Pass tracing to exception.
**	 1-Dec-03 (gordy)
**	    Moved segment handling to startSegment().
*/

public void
write( byte bytes[], int offset, int length )
    throws IOException
{
    try
    {
	for( int end = offset + length; offset < end; offset += length )
	{
	    /*
	    ** A data segment requires at least three bytes
	    ** of space: two for the length indicator and
	    ** one for the data.
	    */
	    if ( out.avail() < 3 )  startSegment();
	    length = Math.min( out.avail(), end - offset );
	    out.write( bytes, offset, length );
	}
    }
    catch( SQLException ex )
    { 
	if ( trace.enabled( 1 ) )
	{
	    trace.write( title + ": exception writing data" );
	    SqlExFactory.trace(ex, trace );
	}

	MsgOut.this.disconnect();
	throw new IOException( ex.getMessage() );
    }

    return;
} // write


/*
** Name: close
**
** Description:
**	Close the output stream.
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
**	12-Apr-01 (gordy)
**	    Pass tracing to exception.
*/

public void
close()
    throws IOException
{
    try { end(); }
    catch( SQLException ex )
    { 
	if ( trace.enabled( 1 ) )
	{
	    trace.write( title + ": exception writing end-of-segments" );
	    SqlExFactory.trace(ex, trace );
	}

	MsgOut.this.disconnect();
	throw new IOException( ex.getMessage() );
    }

    return;
}


/*
** Name: end
**
** Description:
**	End the current output stream of segments.  The end-
**	of-segments indicator is appended to the output buffer.
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
**	 1-Dec-03 (gordy)
**	    Starting a new segment results in 0 length segment
**	    which is the end-of-segment indicator.
*/

public void
end()
    throws SQLException
{
    if ( msg_id >= 0 )
    {
	/*
	** Clean-up any prior segment.  The temporary
	** length indicator generated by startSegment()
	** is exactly what is needed as end-of-segment
	** indicator.
	*/
	startSegment();
	msg_id = -1;
	li_pos = -1;

	if ( trace.enabled( 3 ) )  trace.write( title + ": end of BLOB" );
    }

    return;
} // end


/*
** Name: startSegment
**
** Description:
**	Start a data segment.  A temporary zero length indicator
**	(end-of-segments) is written as a place holder for the 
**	new segment.  If a prior segment exists, the prior segment 
**	length indicator is updated.
**
**	The current message is split if necessary.
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
**	 1-Dec-03 (gordy)
**	    Created.
*/

private void
startSegment()
    throws SQLException
{
    if ( li_pos >= 0 )
    {
	/*
	** Determine current segment length, allowing
	** for size of segment length indicator.
	*/
	int length = (out.position() - li_pos) - 2;
	
	/*
	** If current segment is empty, use it as the
	** new segment since empty data segments are 
	** not permitted.
	*/
	if ( length <= 0 )  return;
	
	/*
	** Update BLOB segment length indicator.
	*/
	if ( trace.enabled( 4 ) )
	    trace.write( title + ": BLOB segment length " + length );
	out.write( li_pos, (short)length );
    }

    /*
    ** If there isn't sufficient room in the current
    ** message for a data segment, next segment will
    ** need to be in next message segment.
    */
    if ( out.avail() < 3 )
    {
        MsgOut.this.done( false );	    // Finish current message
        MsgOut.this.begin( msg_id );    // Begin new message of same type.
    }

    /*
    ** Save the current length indicator position 
    ** so that it may be updated later.  Write an
    ** end-of-segments (zero length) indicator as
    ** a place holder.
    */
    li_pos = out.position();
    out.write( (short)0 );
    return;
} // startSegment


} // class ByteSegOS


/*
** Name: Ucs2SegWtr
**
** Description:
**	Class for converting character stream into segmented writer
**	stream.
**
**	This class is designed for repeated usage.  The begin() and 
**	end() methods provide the means for a single class instance 
**	to process multiple serial BLOBs on the same connection.  
**
**	Note that the close method does not actually close the Writer 
**	object, but simply ends the current BLOB output stream (same 
**	as end()).
**
**  Public Methods
**
**	begin		Initialize processing of a BLOB.
**	write		Write bytes in BLOB stream.
**	flush		Flush output.
**	close		Close current BLOB stream.
**	end		Complete processing of a BLOB.
**
**  Private Data
**
**	title		Class title for tracing.
**	msg_id		Message ID of message containing the BLOB.
**	li_pos		Position of segment length indicator in output buffer.
**
**  Private Methods:
**
**	startSegment	Start a new data segment.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

private class
Ucs2SegWtr
    extends Writer
{

    private String	title;			// Class title for tracing.

    private byte	msg_id = -1;		// Current message ID.
    private int		li_pos = -1;		// Position of length indicator.


/*
** Name: Ucs2SegWtr
**
** Description:
**	Class constructor.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public
Ucs2SegWtr()
{
    title = "Ucs2SegWtr[" + MsgOut.this.connID() + "]";
} // Ucs2SegWtr


/*
** Name: begin
**
** Description:
**	Initialize the segmentor for a new series of BLOB segments.
**	The current message ID must be provided since segmentation 
**	of the BLOB may require the current message to be completed 
**	and a new message of the same type to be started.
**
** Input:
**	msg_id	    ID of current message.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
begin( byte msg_id )
    throws SQLException
{
    this.msg_id = msg_id;
    li_pos = -1;
    
    if ( trace.enabled( 3 ) )  trace.write( title + ": start of CLOB" );
    startSegment();
    return;
}


/*
** Name: write
**
** Description:
**	Write a char sub-array to the segment stream.  The remainder
**	of the current message buffer is used to build a segment.
**	When the buffer is filled, the current message is completed
**	and a new message of the same type is started.
**
** Input:
**	chars	Char array.
**	offset	Starting char to be output.
**	length	Number of chars to be output.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
write( char chars[], int offset, int length )
    throws IOException
{
    try
    {
	for( int end = offset + length; offset < end; offset += length )
	{
	    /*
	    ** A data segment requires at least four bytes
	    ** of space: two for the length indicator and
	    ** two for the data.
	    */
	    if ( out.avail() < 4 )  startSegment();
	    length = Math.min( out.avail() / 2, end - offset );
	    MsgOut.this.writeUCS2( chars, offset, length );
	}
    }
    catch( SQLException ex )
    { 
	if ( trace.enabled( 1 ) )
	{
	    trace.write( title + ": exception writing data" );
	    SqlExFactory.trace(ex, trace );
	}

	MsgOut.this.disconnect();
	throw new IOException( ex.getMessage() );
    }

    return;
} // write


/*
** Name: flush
**
** Description:
**	Flush the output stream.
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
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
flush()
    throws IOException
{
    return;	// Nothing to do!
} // flush


/*
** Name: close
**
** Description:
**	Close the output stream.
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
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
close()
    throws IOException
{
    try { end(); }
    catch( SQLException ex )
    { 
	if ( trace.enabled( 1 ) )
	{
	    trace.write( title + ": exception writing end-of-segments" );
	    SqlExFactory.trace(ex, trace );
	}

	MsgOut.this.disconnect();
	throw new IOException( ex.getMessage() );
    }

    return;
}


/*
** Name: end
**
** Description:
**	End the current output stream of segments.  The end-
**	of-segments indicator is appended to the output buffer.
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
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
end()
    throws SQLException
{
    if ( msg_id >= 0 )
    {
	/*
	** Clean-up any prior segment.  The temporary
	** length indicator generated by startSegment()
	** is exactly what is needed as end-of-segment
	** indicator.
	*/
	startSegment();
	msg_id = -1;
	li_pos = -1;

	if ( trace.enabled( 3 ) )  trace.write( title + ": end of CLOB" );
    }

    return;
} // end


/*
** Name: startSegment
**
** Description:
**	Start a data segment.  A temporary zero length indicator
**	(end-of-segments) is written as a place holder for the 
**	new segment.  If a prior segment exists, the prior segment 
**	length indicator is updated.
**
**	The current message is split if necessary.
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
**	 1-Dec-03 (gordy)
**	    Created.
*/

private void
startSegment()
    throws SQLException
{
    if ( li_pos >= 0 )
    {
	/*
	** Determine current segment length, allowing
	** for size of segment length indicator.
	*/
	int length = ((out.position() - li_pos) - 2) / 2;
	
	/*
	** If current segment is empty, use it as the
	** new segment since empty data segments are 
	** not permitted.
	*/
	if ( length <= 0 )  return;
	
	/*
	** Update BLOB segment length indicator.
	*/
	if ( trace.enabled( 4 ) )
	    trace.write( title + ": CLOB segment length " + length );
	out.write( li_pos, (short)length );
    }

    /*
    ** If there isn't sufficient room in the current
    ** message for a data segment, next segment will
    ** need to be in next message segment.
    */
    if ( out.avail() < 4 )
    {
        MsgOut.this.done( false );	    // Finish current message
        MsgOut.this.begin( msg_id );    // Begin new message of same type.
    }

    /*
    ** Save the current length indicator position 
    ** so that it may be updated later.  Write an
    ** end-of-segments (zero length) indicator as
    ** a place holder.
    */
    li_pos = out.position();
    out.write( (short)0 );
    return;
} // startSegment


} // class Ucs2SegWtr

} // class MsgOut

