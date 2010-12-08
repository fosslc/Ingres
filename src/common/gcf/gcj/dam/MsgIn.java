/*
** Copyright (c) 1999, 2010 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.dam;

/*
** Name: MsgIn.java
**
** Description:
**	Defines class providing the capability to receive messages
**	from a Data Access Server.
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
**	MsgIn		Extends MsgOut.
**	ByteSegIS	Segmented byte BLOB input stream.
**	Ucs2SegRdr	Segmented UCS2 BLOB reader.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	10-Sep-99 (gordy)
**	    Parameterized the initial TL connection data.
**	13-Sep-99 (gordy)
**	    Implemented error code support.
**	22-Sep-99 (gordy)
**	    Added character set/encoding.
**	29-Sep-99 (gordy)
**	    Added Desegmentor class to process BLOBs as streams.  
**	    Added methods (readBytes(),readBinary(),readUnicode(),
**	    skip()) to process BLOBs.
**	17-Nov-99 (gordy)
**	    Extracted input functionality from DbConn.java.
**	25-Jan-00 (gordy)
**	    Added tracing.
**	21-Apr-00 (gordy)
**	    Moved to package io.
**	12-Apr-01 (gordy)
**	    Pass tracing to exception.
**	18-Apr-01 (gordy)
**	    Added readBytes() method which reads the length from message.
**	10-May-01 (gordy)
**	    Support UCS2 character values.  Added UCS2 segmented BLOB class.
**	 6-Sep-02 (gordy)
**	    Character encoding now encapsulated in CharSet class.
**	    Moved to GCF dam package.  Renamed as DAM Message
**	    Layer implementation class.
**	31-Oct-02 (gordy)
**	    Renamed GCF error codes.
**	20-Dec-02 (gordy)
**	    Message header ID protocol level dependent.  TL protocol level
**	    communicated to rest of messaging system.  New connection param
**	    for character set ID.  Enhanced character set/encoding processing.
**	15-Jan-03 (gordy)
**	    Added MSG layer ID connection parameter.
**	22-Sep-03 (gordy)
**	    Added support for ByteArray, CharArray, and SqlData values.
**	 6-Oct-03 (gordy)
**	    Added ability to check for end-of-group.
**	 1-Dec-03 (gordy)
**	    Final cleanup of SQL data values.
**	27-Feb-04 (gordy)
**	    Use ASCII encoding for character-set parameter.
**	15-Mar-04 (gordy)
**	    Support SQL BIGINT and Java long values.
**	16-Jun-06 (gordy)
**	    ANSI Date/Time data type support.
**	 3-Jul-06 (gordy)
**	    Provided alternate way to override character encoding.
**	15-Nov-06 (gordy)
**	    LOB Locator support.
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
**	    Added support for BOOLEAN data type.
**	25-Mar-10 (gordy)
**	    Added support for batch processing.
*/

import	java.io.InputStream;
import	java.io.InputStreamReader;
import	java.io.Reader;
import	java.io.IOException;
import	java.io.UnsupportedEncodingException;
import	java.sql.SQLException;
import	com.ingres.gcf.util.CharSet;
import	com.ingres.gcf.util.IdMap;
import	com.ingres.gcf.util.ByteArray;
import	com.ingres.gcf.util.CharArray;
import	com.ingres.gcf.util.SqlNull;
import	com.ingres.gcf.util.SqlTinyInt;
import	com.ingres.gcf.util.SqlSmallInt;
import	com.ingres.gcf.util.SqlInt;
import	com.ingres.gcf.util.SqlBigInt;
import	com.ingres.gcf.util.SqlReal;
import	com.ingres.gcf.util.SqlDouble;
import	com.ingres.gcf.util.SqlDecimal;
import	com.ingres.gcf.util.SqlBool;
import	com.ingres.gcf.util.IngresDate;
import	com.ingres.gcf.util.SqlDate;
import	com.ingres.gcf.util.SqlTime;
import	com.ingres.gcf.util.SqlTimestamp;
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
import	com.ingres.gcf.util.SqlLoc;
import	com.ingres.gcf.util.SqlStream;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.util.GcfErr;
import	com.ingres.gcf.util.Trace;
import	com.ingres.gcf.util.TraceLog;


/*
** Name: MsgIn
**
** Description:
**	Class providing methods for receiving messages from a Data 
**	Access Server.  Provides methods for reading messages of
**	the Message Layer of the Data Access Messaging protocol 
**	(DAM-ML).  Processes Transport Layer (DAM-TL) packets from
**	the server using the buffered input class InBuff
**
**	The DAM-ML connection parameters received from the server
**	are provided as output of the class constructor.
**
**	When reading messages, the caller must provide multi-
**	threaded protection for each message.
**
**  Public Methods:
**
**	receive			Load the next input message.
**	moreData		More data in message?
**	isEndOfBatch		Message is EOB?
**	moreMessages		More messages follow current message?
**	skip			Skip bytes in input message.
**	readByte		Read a byte from input message.
**	readShort		Read a short value from input message.
**	readInt			Read an integer value from input message.
**	readFloat		Read a float value from input message.
**	readDouble		Read a double value from input message.
**	readBytes		Read a byte array from input message.
**	readString		Read a string value from input message.
**	readUCS2		Read a UCS2 string value from input message.
**	readByteStream		Read a segmented byte stream.
**	readUCS2Reader		Read a segmented character stream.
**	readSqlData		Read an SQL data value.
**
**  Protected Methods:
**
**	connect			Connect with target server.
**	recvCC			Receive connection confirmation.
**	disconnect		Disconnect from the server.
**	close			Close the server connection.
**	setBuffSize		Set the I/O buffer size.
**	setIoProtoLvl		Set the I/O protocol level.
**
**  Private Data:
**
**	in			Input buffer.
**	in_msg_id		Current input message ID.
**	in_msg_len		Input message length.
**	in_msg_flg		Input message flags.
**	empty			Zero length strings.
**	no_params		No DAM-ML connection parameters.
**
**  Private Methods:
**
**	serverDisconnect	Process server disconnect request.
**	readSqlDataIndicator	Read the SQL data/NULL inidicator byte.
**	need			Make sure data is available in input message.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	10-Sep-99 (gordy)
**	    Parameterized the initial TL connection data.
**	22-Sep-99 (gordy)
**	    Added character set/encoding.
**	29-Sep-99 (gordy)
**	    Added methods (readBytes(),readBinary(),readUnicode(),
**	    skip()) to process BLOBs.
**	17-Nov-99 (gordy)
**	    Extracted input functionality from DbConn.
**	18-Apr-01 (gordy)
**	    Added readBytes() method which reads the length from message.
**	10-May-01 (gordy)
**	    Added readUCS2() methods and char_buff to support UCS2
**	    character strings, along with a UCS2 segmented BLOB class.
**	 1-Oct-02 (gordy)
**	    Renamed as DAM-ML messaging class.  Added empty.
**	20-Dec-02 (gordy)
**	    Added setIoProtoLvl() to set the TL protocol level.  Extracted
**	    character set/encoding processing to getCharSet().
**	22-Sep-03 (gordy)
**	    Added readBytes() variant to support ByteArray values,
**	    readUCS2() variant to support CharArray values, and
**	    readSqlData(), readSqlDataIndicator() to support SQL 
**	    data values.  Dropped readBinary(), readAscii(), and 
**	    readUnicode() (BLOBs only supported as SQL data values).
**	    Removed segIS and segRdr (new objects created for each
**	    BLOB).
**	 6-Oct-03 (gordy)
**	    Added moreMessages().
**	 1-Dec-03 (gordy)
**	    Removed unused char_buff and readUCS2() methods which were  
**	    replaced by readSqlData() methods added previously.
**	15-Mar-04 (gordy)
**	    Added methods to read long (readLong()) and SqlBigInt
**	    (readSqlData()) values.
**	 3-Jul-06 (gordy)
**	    Dropped getCharset() and in-lined code in constructor.
**	15-Nov-06 (gordy)
**	    Added readSqlData() method to read LOB Locator  values.
**	    Added readByteStream() and readUCS2Reader() to provide 
**	    generalized access to segmented data streams.  Added
**	    readUCS2() methods to return String.
**	26-Feb-07 (gordy)
**	    Added readSqlData() methods to read buffered LOBs.
**	 5-Dec-07 (gordy)
**	    Extract connection functionality from constructor to
**	    connect() and recvCC().  Added static default for no
**	    DAM-ML connection parameters.
**	26-Oct-09 (gordy)
**	    Added readSqlData() method for boolean.
**	25-Mar-10 (gordy)
**	    Added isEndOfBatch().
*/

class
MsgIn
    extends MsgOut
    implements TlConst, MsgConst, GcfErr
{

    /*
    ** Input buffer and BLOB streams.
    */
    private InBuff		in = null;	// Input buffer.

    /*
    ** Current message info
    */
    private byte		in_msg_id = 0;	// Input message ID.
    private short		in_msg_len = 0;	// Input message length.
    private byte		in_msg_flg = 0;	// Input message flags.

    private static String	empty = "";	// Zero-length strings
    private static byte		no_params[] = new byte[0]; // No DAM-ML params

    
/*
** Name: MsgIn
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
**	    Extracted connect() and recvCC() functionality.
*/

protected
MsgIn( TraceLog log )
    throws SQLException
{
    super( log );
    title = "MsgIn[" + connID() + "]";
}


/*
** Name: connect
**
** Description:
**	Establish connection with target server.  Initializes 
**	the input buffer.
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
**	    Extracted input functionality from DbConn.
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
	in = new InBuff( socket.getInputStream(), 
			 connID(), 1 << DAM_TL_PKT_MIN, trace.getTraceLog() ); 
    }
    catch( Exception ex )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( title + ": error creating input buffer: " + 
			 ex.getMessage() );
	disconnect();
	throw SqlExFactory.get( ERR_GC4001_CONNECT_ERR, ex );
    }

    return;
} // connect


/*
** Name: recvCC
**
** Description:
**	Receives the DAM-TL Connection Confirm packet.  DAM-ML connection 
**	parameters are returned
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	byte []		DAM-ML connection parameters.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	10-Sep-99 (gordy)
**	    Parameterized the initial TL connection data.
**	22-Sep-99 (gordy)
**	    Added character set/encoding.
**	 6-Sep-02 (gordy)
**	    Character encoding now encapsulated in CharSet class.
**	    Read character set during connection parameter processing
**	    and leave mapping to encoding for parameter validation.
**	    Trace character set and encoding mapping.
**	 1-Oct-02 (gordy)
**	    Renamed as DAM Message Layer.  Replaced connection info 
**	    parameters with multi-dimensional array: input passed to
**	    super-class, response returned as output.  Added trace log 
**	    parameter.
**	20-Dec-02 (gordy)
**	    Added char_set parameter for connection character encoding.
**	    Communicate TL protocol level to rest of messaging system.
**	    Added conn param for char-set ID.  Check for server abort.
**	15-Jan-03 (gordy)
**	    Added MSG layer ID connection parameter.
**	27-Feb-04 (gordy)
**	    Use ASCII encoding for character-set parameter to avoid
**	    bootstrapping problems.
**	 3-Jul-06 (gordy)
**	    Character-set may be overridden using setCharSet().
**	    Dropped char_set parameter, don't default CharSet.
**	 5-Dec-07 (gordy)
**	    Extracted from constructor.
*/

protected byte[]
recvCC()
    throws SQLException
{
    String  cs = null;			// Character set.
    int	    cs_id = -1;			// Character set ID.
    int	    size = DAM_TL_PKT_MIN;	// TL packet size.
    short   tl_id;
    byte    msg_cp[] = no_params;	// No output yet.

    try
    {
	if ( trace.enabled( 3 ) )
	    trace.write( title + ": confirm TL connect" );

	/*
	** A Connection Confirmation packet is expected.  Any
	** other response will throw an exception (perhaps
	** after processing a server disconnect request).
	*/
	switch( (tl_id = in.receive()) )
	{
	case DAM_TL_CC :  /* OK */		break;
	case DAM_TL_DR :  serverDisconnect();	return( no_params );
	default :
	    if ( trace.enabled( 1 ) )  
		trace.write( title + ": invalid TL CC packet ID " + tl_id );
	    throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	}
    
	/*
	** Read negotiated values from server (if available).
	*/
	while( in.avail() >= 2 )
	{
	    byte    param_id = in.readByte();
	    short   param_len = (short)(in.readByte() & 0xff);

	    if ( param_len == 255 )
		if ( in.avail() >= 2 )
		    param_len = in.readShort();
		else
		    throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );

	    if ( in.avail() < param_len )
		throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );

	    switch( param_id )
	    {
	    case DAM_TL_CP_PROTO :
		if ( param_len == 1 )  
		    tl_proto_lvl = in.readByte();
		else
		    throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
		break;

	    case DAM_TL_CP_SIZE :
		switch( param_len )
		{
		case 1 :  size = in.readByte();		break;
		case 2 :  size = in.readShort();	break;
		case 4 :  size = in.readInt();		break;

		default :
		    throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
		}
		break;

	    case DAM_TL_CP_CHRSET :
		/*
		** This parameter is availabe at all protocol levels.
		** Due to the inherent bootstrapping problem of providing
		** the character-set as a character string, the parameter
		** value is restricted to ASCII characters only.
		*/
		{
		    byte ba[] = new byte[ param_len ];
		    
		    if ( in.readBytes( ba, 0, param_len ) == param_len )
			try { cs = new String( ba, "US-ASCII" ); }
			catch( UnsupportedEncodingException ex )
			{ throw SqlExFactory.get( ERR_GC401E_CHAR_ENCODE ); }
		    else
			throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
		}
		break;

	    case DAM_TL_CP_CHRSET_ID :
		/*
		** This parameter avoids the bootstrapping problem 
		** which exists with the character-set name, but is 
		** only availabe starting at protocol level 2.
		*/
		if ( tl_proto_lvl < DAM_TL_PROTO_2 )
		    throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );

		switch( param_len )
		{
		case 1 :  cs_id = in.readByte();	break;
		case 2 :  cs_id = in.readShort();	break;
		case 4 :  cs_id = in.readInt();		break;

		default :
		    throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
		}
		break;

	    case DAM_TL_CP_MSG :
		msg_cp = new byte[ param_len ];
		if ( in.readBytes( msg_cp, 0, param_len ) != param_len )
		    throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );		
		break;

	    case DAM_TL_CP_MSG_ID :
		if ( param_len != 4  ||  in.readInt() != DAM_TL_MSG_ID )
		    throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
		break;

	    default :
		if ( trace.enabled( 1 ) )  
		    trace.write( title + ": Bad TL CC param ID: " + param_id );
		throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	    }
	}

	if ( in.avail() > 0 )  throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );

	/*
	** Validate message parameters.
	*/
	if ( 
	     tl_proto_lvl < DAM_TL_PROTO_1  ||  
	     tl_proto_lvl > DAM_TL_PROTO_DFLT  ||
	     size < DAM_TL_PKT_MIN  ||  
	     size > DAM_TL_PKT_MAX 
	   )
	{
	    if ( trace.enabled( 1 ) )
		trace.write( title + ": invalid TL parameter: protocol " + 
			  tl_proto_lvl + ", size " + size );
	    throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	}
    }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( title + ": error negotiating parameters" );
	disconnect();
	throw ex;
    }

    /*
    ** Connection character encoding mapped from character-
    ** set ID or name.  Mapping errors are ignored to allow 
    ** an explicit encoding to be configured.
    */
    if ( cs_id >= 0 )  
	try { this.char_set = CharSet.getCharSet( cs_id ); }
	catch( Exception ex ) 
	{
	    if ( trace.enabled( 1 ) )
		trace.write( title + ": unknown character-set: " + cs_id );
	}
	
    if ( this.char_set == null  &&  cs != null )
	try { this.char_set = CharSet.getCharSet( cs ); }
	catch( Exception ex )
	{
	    if ( trace.enabled( 1 ) )
		trace.write( title + ": unknown character-set: " + cs );
	}

    /*
    ** We initialized with minimum buffer size, then requested
    ** maximum size for the connection.  If the server accepts
    ** anything more than minimum, adjust the buffers accordingly.
    */
    if ( size != DAM_TL_PKT_MIN )  setBuffSize( 1 << size );
    setIoProtoLvl( tl_proto_lvl );

    if ( trace.enabled( 2 ) )
	trace.write( title + ": TL connection opened" );

    if ( trace.enabled( 3 ) )
    {
	trace.write( "    TL protocol level : " + tl_proto_lvl );
	trace.write( "    TL buffer size    : " + (1 << size) );
	trace.write( "    Character encoding: " + this.char_set );
    }

    return( msg_cp );
} // recvCC


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
**	    Extracted input functionality from DbConn.
*/

protected void
disconnect()
{
    /*
    ** We don't set the input buffer reference to null
    ** here so that we don't have to check it on each
    ** use.  I/O buffer functions will continue to work
    ** until a request results in a stream I/O request,
    ** in which case an exception will be thrown by the
    ** I/O buffer.
    **
    ** We must, however, test the reference for null
    ** since we may be called by the constructor with
    ** a null input buffer.
    */
    if ( in != null )
    {
	try { in.close(); }
	catch( Exception ignore ) {}
    }

    super.disconnect();
    return;
} // disconnect


/*
** Name: close
**
** Description:
**	Close the connection with the server.
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
**	    Extracted input functionality from DbConn.
**	26-Dec-02 (gordy)
**	    Check for server abort during processing.
*/

protected void
close()
{
    super.close();

    try 
    { 
	if ( trace.enabled( 3 ) )
	    trace.write( title + ": confirm TL disconnect" );

	/*
	** Receive and discard messages until Disconnection Confirm 
	** is received or exception is thrown.
	*/
      loop :
	for(;;)
	    switch( in.receive() )
	    {
	    case DAM_TL_DC :  /* We be done! */		break loop;
	    case DAM_TL_DR :  serverDisconnect();	break;
	    }

	if ( trace.enabled( 2 ) )
	    trace.write( title + ": TL connection closed" );
    }
    catch( SQLException ignore ) 
    {
	if ( trace.enabled( 2 ) )
	    trace.write( title + ": TL connection aborted" );
    }
    
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
    in.setBuffSize( size );
    super.setBuffSize( size );
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
    in.setProtoLvl( proto_lvl );
    super.setIoProtoLvl( proto_lvl );
    return;
} // setIoProtoLvl


/*
** Name: serverDisconnect
**
** Description:
**	Process a TL DR packet for a server disconnect request.
**	Throws an exception indicating connection aborted.
**
**	A Disconnect Confirmation packet should be returned to
**	the server, but there is no reasonable way to do so.
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
**	26-Dec-02 (gordy)
**	    Created.
**	15-Jan-03 (gordy)
**	    Handle all integer parameter value lengths.
**	 5-Dec-07 (gordy)
**	    Provide better handling for common errors.
*/

private void
serverDisconnect()
    throws SQLException
{
    /*
    ** An error code may be provided as an optional parameter.
    */
    if ( in.avail() >= 2 )
    {
	byte    param_id = in.readByte();
	byte    param_len = in.readByte();
	
	if ( param_id == DAM_TL_DP_ERR  &&  in.avail() >= param_len ) 
	{
	    int error;
	    
	    switch( param_len )
	    {
	    case 1 : error = in.readByte();	break;
	    case 2 : error = in.readShort();	break;
	    case 4 : error = in.readInt();	break;
	    default: error = E_GC4008_SERVER_ABORT;	break;
	    }

	    if ( trace.enabled( 1 ) )  
		trace.write( title + ": server error 0x" + 
				     Integer.toHexString( error ) );

	    switch( error )
	    {
	    case E_GC4008_SERVER_ABORT :
		throw SqlExFactory.get( ERR_GC4008_SERVER_ABORT );

	    case E_GC480E_CLIENT_MAX :
		throw SqlExFactory.get( ERR_GC480E_CLIENT_MAX );

	    case E_GC481F_SVR_CLOSED :
		throw SqlExFactory.get( ERR_GC481F_SVR_CLOSED );

	    default :
		/*
		** Need to throw a SERVER_ABORT exception, but need the 
		** server error code instead of the driver error code. 
		** Generate a driver exception and use the message text 
		** and SQLSTATE along with server error code to generate 
		** the actual exception.
		*/
		SQLException sqlEx = SqlExFactory.get( ERR_GC4008_SERVER_ABORT );
		throw new SQLException( sqlEx.getMessage(), 
				 sqlEx.getSQLState(), error );
	    }
	}
    }

    /*
    ** Server failed to provided an abort code.
    */
    if ( trace.enabled( 1 ) )  trace.write( title + ": server abort" );
    throw SqlExFactory.get( ERR_GC4008_SERVER_ABORT );
} // serverDisconnect


/*
** Name: receive
**
** Description:
**	Read the next input message.  Message may be present in the
**	input buffer (message concatenation) or will be read from
**	input stream.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	byte	    Message ID.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	10-Sep-99 (gordy)
**	    Removed test for TL DR (now done in in.receive()).
**	20-Dec-02 (gordy)
**	    Header ID protocol level dependent.  Restored handling TL DR.
**	25-Mar-10 (gordy)
**	    Added EOB header flag.
*/

public byte
receive()
    throws SQLException
{
    int msg_id;

    if ( trace.enabled( 3 ) )  trace.write( title + ": check TL data" );

    /*
    ** Check to see if message header is in the input
    ** buffer (handles message concatenation).
    */
    while( in.avail() < 8 )
	try 
	{ 
	    short tl_id;

	    /*
	    ** Headers are not split across continued messages.
	    ** Data remaining in the current buffer indicates
	    ** a message processing error.
	    */
	    if ( in.avail() > 0 )
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( title + ": invalid header length " + 
				 in.avail() + " bytes" );
		throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	    }

	    switch( (tl_id = in.receive()) )
	    {
	    case DAM_TL_DT :  /* OK */			break;
	    case DAM_TL_DR :  serverDisconnect();	break;
	    default :
		if ( trace.enabled( 1 ) )  
		    trace.write( title + ": invalid TL packet ID 0x" +
				 Integer.toHexString( tl_id ) );
		throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	    }
	}
	catch( SQLException ex )
	{
	    disconnect();
	    throw ex;
	}

    /*
    ** Read and validate message header.
    */
    msg_id = in.readInt();

    if ( 
	 (msg_proto_lvl < MSG_PROTO_3  &&  msg_id != MSG_HDR_ID_1)  ||
	 (msg_proto_lvl >= MSG_PROTO_3  &&  msg_id != MSG_HDR_ID_2)
       )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( title + ": invalid header ID 0x" +
			 Integer.toHexString( msg_id ) );
	disconnect();
	throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
    }

    in_msg_len = in.readShort();
    in_msg_id = in.readByte();
    in_msg_flg = in.readByte();

    if ( trace.enabled( 2 ) )  
	trace.write( title + ": received message " + 
		     IdMap.map( in_msg_id, msgMap ) + " length " + in_msg_len + 
		     ((in_msg_flg & MSG_HDR_EOD) == 0 ? "" : " EOD") +
		     ((in_msg_flg & MSG_HDR_EOB) == 0 ? "" : " EOB") +
		     ((in_msg_flg & MSG_HDR_EOG) == 0 ? "" : " EOG") );

    return( in_msg_id );
} // receive


/*
** Name: moreData
**
** Description:
**	Returns an indication that additional data remains in the
**	current input message.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if additional data available, false otherwise.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
*/

public boolean
moreData()
    throws SQLException
{
    byte msg_id = in_msg_id;

    while( in_msg_len <= 0  &&  (in_msg_flg & MSG_HDR_EOD) == 0 )
    {
	/*
	** Receive the message continuation and check that
	** it is a continuation of the current message.
	*/
	if ( receive() != msg_id )
	{
	    if ( trace.enabled( 1 ) )  
		trace.write( title + ": invalid message continuation " +
			     msg_id + " to " + in_msg_id );
	    disconnect();
	    throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	}
    }

    return( in_msg_len > 0 );
} // moreData


/*
** Name: isEndOfBatch
**
** Description:
**	Returns an indication that current message is marked EOB.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean		True if EOB, false otherwise.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

public boolean
isEndOfBatch()
{
    return( (in_msg_flg & MSG_HDR_EOB) != 0 );
} // isEndOfBatch


/*
** Name: moreMessages
**
** Description:
**	Returns an indication that additional messages follow the
**	current message (end-of-group has not been reached).
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if additional messages, false otherwise.
**
** History:
**	 6-Oct-03 (gordy)
**	    Created.
*/

public boolean
moreMessages()
{
    return( (in_msg_flg & MSG_HDR_EOG) == 0 );
} // moreMessages


/*
** Name: skip
**
** Description:
**	Skip bytes in the current input message.
**
** Input:
**	length	    Number of bytes.
**
** Output:
**	None.
**
** Returns:
**	int	    Number of bytes actually skipped.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
*/

public int
skip( int length )
    throws SQLException
{
    int total = 0;

    if ( trace.enabled( 4 ) )
	trace.write( title + ": skipping " + length + " bytes" );

    while( length > 0 )
    {
	int len;
	need( length, false );
	len = Math.min( in_msg_len, length );
	len = in.skip( len );
	length -= len;
	in_msg_len -= len;
	total += len;
    }

    return( total );
} // skip


/*
** Name: readByte
**
** Description:
**	Read a byte value from the current input message.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	byte	Input byte value.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
*/

public byte
readByte()
    throws SQLException
{
    need( 1, true );
    in_msg_len -= 1;
    return( in.readByte() );
} // readByte


/*
** Name: readShort
**
** Description:
**	Read a short value from the current input message.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	short	Input short value.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
*/

public short
readShort()
    throws SQLException
{
    need( 2, true );
    in_msg_len -= 2;
    return( in.readShort() );
} // readShort


/*
** Name: readInt
**
** Description:
**	Read an integer value from the current input message.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Input integer value.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
*/

public int
readInt()
    throws SQLException
{
    need( 4, true );
    in_msg_len -= 4;
    return( in.readInt() );
} // readInt


/*
** Name: readLong
**
** Description:
**	Read a long value from the current input message.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	long	Input long value.
**
** History:
**	15-Mar-04 (gordy)
**	    Created.
*/

public long
readLong()
    throws SQLException
{
    need( 8, true );
    in_msg_len -= 8;
    return( in.readLong() );
} // readLong


/*
** Name: readFloat
**
** Description:
**	Read a float value from the current input message.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	float	Input float value.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
*/

public float
readFloat()
    throws SQLException
{
    need( DAM_TL_F4_LEN, true );
    in_msg_len -= DAM_TL_F4_LEN;
    return( in.readFloat() );
} // readFloat


/*
** Name: readDouble
**
** Description:
**	Read a double value from the current input message.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	double	Input double value.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
*/

public double
readDouble()
    throws SQLException
{
    need( DAM_TL_F8_LEN, true );
    in_msg_len -= DAM_TL_F8_LEN;
    return( in.readDouble() );
} // readDouble


/*
** Name: readBytes
**
** Description:
**	Reads a byte array from the current input message.
**	The length of the array is read from the input message.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	byte[]	    Byte array from message.
**
** History:
**	18-Apr-01 (gordy)
**	    Created.
*/

public byte[]
readBytes()
    throws SQLException
{
    return( readBytes( readShort() ) );
} // readBytes


/*
** Name: readBytes
**
** Description:
**	Reads a byte array from the current input message.
**
** Input:
**	length	    Length of byte array to read.
**
** Output:
**	None.
**
** Returns:
**	byte[]	    Byte array from message
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	29-Jun-99 (gordy)
**	    Created readBytes( byte[], offset, length ) and
**	    simplified this method to create byte array of
**	    desired length and call new readBytes() method.
*/

public byte[]
readBytes( int length )
    throws SQLException
{
    byte    bytes[] = new byte[ length ];
    readBytes( bytes, 0, length );
    return( bytes );
} // readBytes


/*
** Name: readBytes
**
** Description:
**	Reads bytes from the current input message into a
**	byte sub-array.
**
** Input:
**	offset	    Offset in byte array.
**	length	    Number of bytes.
**
** Output:
**	bytes	    Bytes read from message.
**
** Returns:
**	int	    Number of bytes actually read.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
*/

public int
readBytes( byte bytes[], int offset, int length )
    throws SQLException
{
    int requested = length;

    while( length > 0 )
    {
	need( length, false );
	int len = Math.min( (int)in_msg_len, length );
	len = in.readBytes( bytes, offset, len );
	offset += len;
	length -= len;
	in_msg_len -= len;
    }

    return( requested - length );
} // readBytes


/*
** Name: readBytes
**
** Description:
**	Reads bytes from the current input message into a
**	ByteArray.  The array limit is ignored.  Any bytes
**	which exceed the limit are read and discarded.
**	It is the callers responsible to ensure adaquate 
**	limit and capacity.
**
** Input:
**	offset	    Offset in byte array.
**	length	    Number of bytes.
**
** Output:
**	bytes	    Filled with bytes from message.
**
** Returns:
**	int	    Number of bytes actually read.
**
** History:
**	22-Sep-03 (gordy)
**	    Created.
*/

public int
readBytes( ByteArray bytes, int length )
    throws SQLException
{
    int requested = length;
    
    while( length > 0 )
    {
	need( length, false );
	int len = Math.min( (int)in_msg_len, length );
	len = in.readBytes( bytes, len );
	length -= len;
	in_msg_len -= len;
    }
    
    return( requested - length );
} // readBytes


/*
** Name: readString
**
** Description:
**	Reads a string value from the current input message.
**	The length of the string is read from the input stream.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	    String read from message.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
*/

public String
readString()
    throws SQLException
{
    return( readString( readShort() ) );
} // readString


/*
** Name: readString
**
** Description:
**	Reads a string value from the current input message.
**
** Input:
**	length	    Length of string.
**
** Output:
**	None.
**
** Returns:
**	String	    String read from message.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	22-Sep-99 (gordy)
**	    Added character set/encoding.
**	 6-Sep-02 (gordy)
**	    Moved character encoding conversion to CharSet class.
*/

public String
readString( int length )
    throws SQLException
{
    String  str;

    if ( length <= in_msg_len )	    // Is entire string available?
	if ( length <= 0 )
	    str = empty;
	else
	{
	    str = in.readString( length, char_set );
	    in_msg_len -= length;
	}
    else
    {
	/*
	** Entire string is not available.  Collect the fragments
	** (readBytes() does this automatically) and convert to string.
	*/
	byte buff[] = readBytes( length );

	try { str = char_set.getString( buff ); }
	catch( Exception ex )
	{
	    throw SqlExFactory.get( ERR_GC401E_CHAR_ENCODE );	// Should not happen!
	}
    }

    return( str );
} // readString


/*
** Name: readUCS2
**
** Description:
**	Reads a UCS2 string value from the current input message.
**	The length of the string is read from the input stream.
**
**	Note: characters are read/appended individually due
**	to the need for conversion from network two byte int
**	to local char.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	    String read from message.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public String
readUCS2()
    throws SQLException
{
    return( readUCS2( readShort() ) );
} // readUCS2


/*
** Name: readUCS2
**
** Description:
**	Reads a UCS2 string value from the current input message.
**
**	Note: characters are read/appended individually due
**	to the need for conversion from network two byte int
**	to local char.
**
** Input:
**	length	Length of string.
**
** Output:
**	None.
**
** Returns:
**	String	String read from message.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public String
readUCS2( short length )
    throws SQLException
{
    if ( length < 1 )  return( empty );

    char ca[] = new char[ length ];
    for( int idx = 0; idx < length; idx++ )  ca[ idx ] = (char)readShort();

    return( new String( ca ) );
} // readUCS2


/*
** Name: readUCS2
**
** Description:
**	Reads UCS2 characters from the current input message
**	into a CharArray.  The array limit is ignored.  Any
**	characters which exceed the array limit are read and
**	discarded.  It is the callers responsibility to 
**	ensure adaquate limit and capacity.
**
**	Note: characters are read/appended individually due
**	to the need for conversion from network two byte int
**	to local char.
**
** Input:
**	length	    Number of characters.
**
** Output:
**	array	    Appended with characters from message.
**
** Returns:
**	int	    Number of characters actually read.
**
** History:
**	22-Sep-03 (gordy)
**	    Created.
*/

public int
readUCS2( CharArray array, int length )
    throws SQLException
{
    for( int count = length; count > 0; count-- )
	array.put( (char)readShort() );
    return( length );
} // readUCS2


/*
** Name: readByteStream
**
** Description:
**	Returns a Byte stream capable of reading a segmented byte
**	data stream from the current input message.  The Byte stream
**	implements the SqlStream StreamSource interface to provide 
**	notification of stream closure events.
**
**	The stream returned may be provided to a subsequent call
**	to the companion method to re-use the stream.
**
**	No data is actually read by this method.  The data stream 
**	must be read to completion before reading any additional 
**	items from the current or subsequent messages.
**
**	A byte stream consists of zero or more segments which 
**	are followed by an end-of-segments indicator.  Each segment 
**	is composed of a two byte integer length (non-zero) followed 
**	by a binary byte array of the indicated length.  The end-of-
**	segments indicator is a two byte integer with value zero.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	InputStream	Byte stream.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public InputStream
readByteStream()
    throws SQLException
{
    return( new ByteSegIS( this, trace, in_msg_id ) );
} // readByteStream


/*
** Name: readByteStream
**
** Description:
**	Returns a Byte stream capable of reading a segmented byte
**	data stream from the current input message.  The Byte stream
**	implements the SqlStream StreamSource interface to provide 
**	notification of stream closure events.
**
**	An existing Byte stream (returned by a previous call to this
**	method) may be provided as input and will be initialized for 
**	the new stream.  The stream returned must be used to read
**	the stream.
**
**	No data is actually read by this method.  The data stream 
**	must be read to completion before reading any additional 
**	items from the current or subsequent messages.
**
**	A byte stream consists of zero or more segments which 
**	are followed by an end-of-segments indicator.  Each segment 
**	is composed of a two byte integer length (non-zero) followed 
**	by a binary byte array of the indicated length.  The end-of-
**	segments indicator is a two byte integer with value zero.
**
** Input:
**	stream		Existing Byte stream.
**
** Output:
**	None.
**
** Returns:
**	InputStream	Byte stream.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public InputStream
readByteStream( InputStream stream )
    throws SQLException
{
    /*
    ** Verify existing stream is correct class.
    */
    if ( stream != null  &&  stream instanceof ByteSegIS )
    {
	try { stream.close(); }	// Make sure prior stream is consumed.
	catch( Exception ignore ) {}

	((ByteSegIS)stream).begin( in_msg_id );
    }
    else
	stream = new ByteSegIS( this, trace, in_msg_id );

    return( stream );
} // readByteStream


/*
** Name: readUCS2Reader
**
** Description:
**	Returns a Reader capable of reading a segmented UCS2 
**	data stream from the current input message.  The Reader 
**	implements the SqlStream StreamSource interface to 
**	provide notification of stream closure events.
**
**	The reader returned may be provided to a subsequent call
**	to the companion method to re-use the reader.
**
**	No data is actually read by this method.  The data 
**	stream must be read to completion before reading any 
**	additional items from the current or subsequent 
**	messages.
**
**	A UCS2 data stream consists of zero or more segments 
**	which are followed by an end-of-segments indicator.  
**	Each segment is composed of a two byte integer length 
**	(non-zero) followed by a UCS2 (two byte integer) array 
**	of the indicated length.  The end-of-segments indicator 
**	is a two byte integer with value zero.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Reader	UCS2 Reader.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public Reader
readUCS2Reader()
    throws SQLException
{
    return( new Ucs2SegRdr( this, trace, in_msg_id ) );
} // readUCS2Reader


/*
** Name: readUCS2Reader
**
** Description:
**	Returns a Reader capable of reading a segmented UCS2 
**	data stream from the current input message.  The Reader 
**	implements the SqlStream StreamSource interface to 
**	provide notification of stream closure events.
**
**	An existing UCS2 Reader (returned by a previous call 
**	to this method) may be provided as input and will be 
**	initialized for the new stream.  The reader returned
**	by this method must be used to read the stream.
**
**	No data is actually read by this method.  The data 
**	stream must be read to completion before reading any 
**	additional items from the current or subsequent 
**	messages.
**
**	A UCS2 data stream consists of zero or more segments 
**	which are followed by an end-of-segments indicator.  
**	Each segment is composed of a two byte integer length 
**	(non-zero) followed by a UCS2 (two byte integer) array 
**	of the indicated length.  The end-of-segments indicator 
**	is a two byte integer with value zero.
**
** Input:
**	reader	Existing UCS2 Reader, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	Reader	UCS2 Reader.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public Reader
readUCS2Reader( Reader reader )
    throws SQLException
{
    if ( reader != null  &&  reader instanceof Ucs2SegRdr )
    {
	try { reader.close(); }	// Make sure prior stream is consumed.
	catch( Exception ignore ) {}

	((Ucs2SegRdr)reader).begin( in_msg_id );
    }
    else
	reader = new Ucs2SegRdr( this, trace, in_msg_id );

    return( reader );
} // readUCS2Reader


/*
** Name: readSqlData
**
** Description:
**	Reads a SqlNull data value from the current input message.
**
**	A SqlNull data value is composed of simply a data indicator
**	byte which must be 0 (NULL data value).
**
** Input:
**	None.
**
** Output:
**	value	SQL data value.
**
** Returns:
**	void.
**
** History:
**	22-Sep-03 (gordy)
**	    Created.
*/

public void
readSqlData( SqlNull value )
    throws SQLException
{
    if ( readSqlDataIndicator() )	// Read data indicator byte.
	throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );	// Should be NULL.
    else
        value.setNull();	// Shouldn't be needed, but just to be safe...
    return;
} // readSqlData


/*
** Name: readSqlData
**
** Description:
**	Reads a SqlTinyInt data value from the current input message.
**
**	A SqlTinyInt data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a one byte integer.
**
** Input:
**	None.
**
** Output:
**	value	SQL data value.
**
** Returns:
**	void.
**
** History:
**	22-Sep-03 (gordy)
**	    Created.
*/

public void
readSqlData( SqlTinyInt value )
    throws SQLException
{
    if ( readSqlDataIndicator() )	// Read data indicator byte.
	value.set( readByte() );	// Read data value.
    else
        value.setNull();		// NULL data value.
    return;
} // readSqlData


/*
** Name: readSqlData
**
** Description:
**	Reads a SqlSmallInt data value from the current input message.
**
**	A SqlSmallInt data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a two byte integer.
**
** Input:
**	None.
**
** Output:
**	value	SQL data value.
**
** Returns:
**	void.
**
** History:
**	22-Sep-03 (gordy)
**	    Created.
*/

public void
readSqlData( SqlSmallInt value )
    throws SQLException
{
    if ( readSqlDataIndicator() )	// Read data indicator byte.
	value.set( readShort() );	// Read data value.
    else
        value.setNull();		// NULL data value.
    return;
} // readSqlData


/*
** Name: readSqlData
**
** Description:
**	Reads a SqlInt data value from the current input message.
**
**	A SqlInt data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a four byte integer.
**
** Input:
**	None.
**
** Output:
**	value	SQL data value.
**
** Returns:
**	void.
**
** History:
**	22-Sep-03 (gordy)
**	    Created.
*/

public void
readSqlData( SqlInt value )
    throws SQLException
{
    if ( readSqlDataIndicator() )	// Read data indicator byte.
	value.set( readInt() );		// Read data value.
    else
        value.setNull();		// NULL data value.
    return;
} // readSqlData


/*
** Name: readSqlData
**
** Description:
**	Reads a SqlBigInt data value from the current input message.
**
**	A SqlBigInt data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by an eight byte integer.
**
** Input:
**	None.
**
** Output:
**	value	SQL data value.
**
** Returns:
**	void.
**
** History:
**	15-Mar-04 (gordy)
**	    Created.
*/

public void
readSqlData( SqlBigInt value )
    throws SQLException
{
    if ( readSqlDataIndicator() )	// Read data indicator byte.
	value.set( readLong() );	// Read data value.
    else
        value.setNull();		// NULL data value.
    return;
} // readSqlData


/*
** Name: readSqlData
**
** Description:
**	Reads a SqlReal data value from the current input message.
**
**	A SqlReal data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a four byte floating
**	point value.
**
** Input:
**	None.
**
** Output:
**	value	SQL data value.
**
** Returns:
**	void.
**
** History:
**	22-Sep-03 (gordy)
**	    Created.
*/

public void
readSqlData( SqlReal value )
    throws SQLException
{
    if ( readSqlDataIndicator() )	// Read data indicator byte.
	value.set( readFloat() );	// Read data value.
    else
        value.setNull();		// NULL data value.
    return;
} // readSqlData


/*
** Name: readSqlData
**
** Description:
**	Reads a SqlDouble data value from the current input message.
**
**	A SqlDouble data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by an eight byte floating
**	point value.
**
** Input:
**	None.
**
** Output:
**	value	SQL data value.
**
** Returns:
**	void.
**
** History:
**	22-Sep-03 (gordy)
**	    Created.
*/

public void
readSqlData( SqlDouble value )
    throws SQLException
{
    if ( readSqlDataIndicator() )	// Read data indicator byte.
	value.set( readDouble() );	// Read data value.
    else
        value.setNull();		// NULL data value.
    return;
} // readSqlData


/*
** Name: readSqlData
**
** Description:
**	Reads a SqlDecimal data value from the current input message.
**
**	A SqlDecimal data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a string which is
**	composed of a two byte integer indicating the byte length
**	of the string followed by a character byte array of the
**	indicated length.
**
** Input:
**	None.
**
** Output:
**	value	SQL data value.
**
** Returns:
**	void.
**
** History:
**	22-Sep-03 (gordy)
**	    Created.
*/

public void
readSqlData( SqlDecimal value )
    throws SQLException
{
    if ( readSqlDataIndicator() )	// Read data indicator byte.
	value.set( readString() );	// Read data value.
    else
        value.setNull();		// NULL data value.
    return;
} // readSqlData


/*
** Name: readSqlData
**
** Description:
**	Reads a SqlBool data value from the current input message.
**
**	A SqlBool data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a one byte integer.
**	Only the low-order bit of the integer is significant:
**	FALSE = 0x00 and TRUE = 0x01.
**
** Input:
**	None.
**
** Output:
**	value	SQL data value.
**
** Returns:
**	void.
**
** History:
**	26-Oct-09 (gordy)
**	    Created.
*/

public void
readSqlData( SqlBool value )
    throws SQLException
{
    if ( readSqlDataIndicator() )	// Read data indicator byte.
	value.set( readByte() );	// Read data value.
    else
        value.setNull();		// NULL data value.
    return;
} // readSqlData


/*
** Name: readSqlData
**
** Description:
**	Reads a SqlDate data value from the current input message.
**
**	A SqlDate data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a string which is
**	composed of a two byte integer indicating the byte length
**	of the string followed by a character byte array of the
**	indicated length.
**
** Input:
**	None.
**
** Output:
**	value	SQL data value.
**
** Returns:
**	void.
**
** History:
**	16-Jun-06 (gordy)
**	    Created.
*/

public void
readSqlData( SqlDate value )
    throws SQLException
{
    if ( readSqlDataIndicator() )	// Read data indicator byte.
	value.set( readString() );	// Read data value.
    else
        value.setNull();		// NULL data value.
    return;
} // readSqlData


/*
** Name: readSqlData
**
** Description:
**	Reads a SqlTime data value from the current input message.
**
**	A SqlTime data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a string which is
**	composed of a two byte integer indicating the byte length
**	of the string followed by a character byte array of the
**	indicated length.
**
** Input:
**	None.
**
** Output:
**	value	SQL data value.
**
** Returns:
**	void.
**
** History:
**	16-Jun-06 (gordy)
**	    Created.
*/

public void
readSqlData( SqlTime value )
    throws SQLException
{
    if ( readSqlDataIndicator() )	// Read data indicator byte.
	value.set( readString() );	// Read data value.
    else
        value.setNull();		// NULL data value.
    return;
} // readSqlData


/*
** Name: readSqlData
**
** Description:
**	Reads a SqlTimestamp data value from the current input message.
**
**	A SqlTimestamp data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a string which is
**	composed of a two byte integer indicating the byte length
**	of the string followed by a character byte array of the
**	indicated length.
**
** Input:
**	None.
**
** Output:
**	value	SQL data value.
**
** Returns:
**	void.
**
** History:
**	16-Jun-06 (gordy)
**	    Created.
*/

public void
readSqlData( SqlTimestamp value )
    throws SQLException
{
    if ( readSqlDataIndicator() )	// Read data indicator byte.
	value.set( readString() );	// Read data value.
    else
        value.setNull();		// NULL data value.
    return;
} // readSqlData


/*
** Name: readSqlData
**
** Description:
**	Reads an IngresDate data value from the current input message.
**
**	An IngresDate data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a string which is
**	composed of a two byte integer indicating the byte length
**	of the string followed by a character byte array of the
**	indicated length.
**
** Input:
**	None.
**
** Output:
**	value	SQL data value.
**
** Returns:
**	void.
**
** History:
**	22-Sep-03 (gordy)
**	    Created.
**	03-Nov-10 (rajus01) SIR 124588, SD issue 147074
**	    IngresDate has special handling for empty dates and the parameter 
**	    oriented setNull() function may do something different than 
**	    desired at this point.  Use the column oriented set(null) method 
**	    instead.
*/

public void
readSqlData( IngresDate value )
    throws SQLException
{
    if ( readSqlDataIndicator() )	// Read data indicator byte.
	value.set( readString() );	// Read data value.
    else
        value.set( (IngresDate) null );		// NULL data value.
    return;
} // readSqlData


/*
** Name: readSqlData
**
** Description:
**	Reads a SqlByte data value from the current input message.
**	Uses the ByteArray interface to load the SqlByte data value.
**
**	A SqlByte data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a binary byte array
**	whose length is detemined by the fixed size of the SqlByte 
**	data value (the ByteArray limit()).
**
** Input:
**	None.
**
** Output:
**	value	SQL data value.
**
** Returns:
**	void.
**
** History:
**	22-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Don't need to ensure capacity of fixed length values.
*/

public void
readSqlData( SqlByte value )
    throws SQLException
{
    if ( ! readSqlDataIndicator() )	// Read data indicator byte.
        value.setNull();		// NULL data value.
    else
    {					// Read data value.
	int length = value.limit();	// Fixed length from meta-data.
	value.clear();
	readBytes( (ByteArray)value, length );
    }
    return;
} // readSqlData


/*
** Name: readSqlData
**
** Description:
**	Reads a SqlVarByte data value from the current input message.
**	Uses the ByteArray interface to load the SqlVarByte data value.
**
**	A SqlVarByte data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a two byte integer length
**	which is followed by a binary byte array of the indicated length.
**
** Input:
**	None.
**
** Output:
**	value	SQL data value.
**
** Returns:
**	void.
**
** History:
**	22-Sep-03 (gordy)
**	    Created.
*/

public void
readSqlData( SqlVarByte value )
    throws SQLException
{
    if ( ! readSqlDataIndicator() )	// Read data indicator byte.
        value.setNull();		// NULL data value.
    else
    {					// Read data value.
	int length = readShort();	// Variable length from message.
	value.clear();
	value.ensureCapacity( length );
	readBytes( (ByteArray)value, length );
    }
    return;
} // readSqlData


/*
** Name: readSqlData
**
** Description:
**	Reads a SqlChar data value from the current input message.
**	Uses the ByteArray interface to load the SqlChar data value.
**
**	A SqlChar data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a character byte 
**	array whose length is detemined by the fixed size of the
**	SqlChar data value (the ByteArray limit()).
**
** Input:
**	None.
**
** Output:
**	value	SQL data value.
**
** Returns:
**	void.
**
** History:
**	22-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Don't need to ensure capacity of fixed length values.
*/

public void
readSqlData( SqlChar value )
    throws SQLException
{
    if ( ! readSqlDataIndicator() )	// Read data indicator byte.
        value.setNull();		// NULL data value.
    else
    {					// Read data value.
	int length = value.limit();	// Fixed length from meta-data.
	value.clear();
	readBytes( (ByteArray)value, length );
    }
    return;
} // readSqlData


/*
** Name: readSqlData
**
** Description:
**	Reads a SqlVarChar data value from the current input message.
**	Uses the ByteArray interface to load the SqlVarChar data value.
**
**	A SqlVarChar data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a two byte integer 
**	length which is followed by a character byte array of the
**	indicated length.
**
** Input:
**	None.
**
** Output:
**	value	SQL data value.
**
** Returns:
**	void.
**
** History:
**	22-Sep-03 (gordy)
**	    Created.
*/

public void
readSqlData( SqlVarChar value )
    throws SQLException
{
    if ( ! readSqlDataIndicator() )	// Read data indicator byte.
        value.setNull();		// NULL data value.
    else
    {					// Read data value.
	int length = readShort();	// Variable length from message.
	value.clear();
	value.ensureCapacity( length );
	readBytes( (ByteArray)value, length );
    }
    return;
} // readSqlData


/*
** Name: readSqlData
**
** Description:
**	Reads a SqlNChar data value from the current input message.
**	Uses the CharArray interface to load the SqlNChar data value.
**
**	A SqlNChar data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a UCS2 (two byte
**	integer) array whose length is detemined by the fixed size 
**	of the SqlNChar data value (the CharArray limit()).
**
** Input:
**	None.
**
** Output:
**	value	SQL data value.
**
** Returns:
**	void.
**
** History:
**	22-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Don't need to ensure capacity of fixed length values.
*/

public void
readSqlData( SqlNChar value )
    throws SQLException
{
    if ( ! readSqlDataIndicator() )	// Read data indicator byte.
        value.setNull();		// NULL data value.
    else
    {					// Read data value.
	int length = value.limit();	// Fixed length from meta-data.
	value.clear();
	readUCS2( (CharArray)value, length );
    }
    return;
} // readSqlData


/*
** Name: readSqlData
**
** Description:
**	Reads a SqlNVarChar data value from the current input message.
**	Uses the CharArray interface to load the SqlNVarChar data value.
**
**	A SqlNVarChar data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a two byte integer length 
**	which is followed by a UCS2 (two byte integer) array of the 
**	indicated length.
**
** Input:
**	None.
**
** Output:
**	value	SQL data value.
**
** Returns:
**	void.
**
** History:
**	22-Sep-03 (gordy)
**	    Created.
*/

public void
readSqlData( SqlNVarChar value )
    throws SQLException
{
    if ( ! readSqlDataIndicator() )	// Read data indicator byte.
        value.setNull();		// NULL data value.
    else
    {					// Read data value.
	int length = readShort();	// Variable length from message.
	value.clear();
	value.ensureCapacity( length );
	readUCS2( (CharArray)value, length );
    }
    return;
} // readSqlData


/*
** Name: readSqlData
**
** Description:
**	Reads a SqlLoc (LOB Locator) data value from the current 
**	input message.
**
**	A locator data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by a four byte integer.
**
** Input:
**	None.
**
** Output:
**	value	SQL data value.
**
** Returns:
**	void.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public void
readSqlData( SqlLoc value )
    throws SQLException
{
    if ( readSqlDataIndicator() )	// Read data indicator byte.
	value.set( readInt() );		// Read data value.
    else
        value.setNull();		// NULL data value.
    return;
} // readSqlData


/*
** Name: readSqlData
**
** Description:
**	Reads a SqlLongByte data value from the current input message.
**	Only a single stream data value may be active at a given time.
**	Attempting to read another stream data value will result in
**	the closure of the preceding stream.
**
**	A SqlLongByte data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by zero or more segments
**	which are then followed by an end-of-segments indicator.  Each
**	segment is composed of a two byte integer length (non-zero) 
**	followed by a binary byte array of the indicated length.  The
**	end-of-segments indicator is a two byte integer with value
**	zero.
**
**	Only the SQL data/NULL indicator is read by this method.  If
**	the SqlLongByte value is non-NULL, an InputStream is provided
**	for reading the segmented data stream.  The InputStream also
**	implements the SqlStream StreamSource interface for providing
**	notification of stream closure events.
**
** Input:
**	None.
**
** Output:
**	value	SQL data value.
**
** Returns:
**	void.
**
** History:
**	22-Sep-03 (gordy)
**	    Created.
**	15-Nov-06 (gordy)
**	    Added readByteStream() to access stream.
*/

public void
readSqlData( SqlLongByte value )
    throws SQLException
{
    if ( readSqlDataIndicator() )	// Read data indicator byte.
	value.set( readByteStream() );
    else
        value.setNull();		// NULL data value.
    return;
} // readSqlData


/*
** Name: readSqlData
**
** Description:
**	Reads a SqlLongByteCache data value from the current input message.
**
**	A SqlLongByte data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by zero or more segments
**	which are then followed by an end-of-segments indicator.  Each
**	segment is composed of a two byte integer length (non-zero) 
**	followed by a binary byte array of the indicated length.  The
**	end-of-segments indicator is a two byte integer with value
**	zero.
**
**	Only the SQL data/NULL indicator is read by this method.  If
**	the SqlLongByte value is non-NULL, an InputStream is provided
**	for reading the segmented data stream.
**
** Input:
**	None.
**
** Output:
**	value	SQL data value.
**
** Returns:
**	void.
**
** History:
**	26-Feb-07 (gordy)
**	    Created.
*/

public void
readSqlData( SqlLongByteCache value )
    throws SQLException
{
    if ( readSqlDataIndicator() )	// Read data indicator byte.
	value.set( readByteStream() );
    else
        value.setNull();		// NULL data value.
    return;
} // readSqlData


/*
** Name: readSqlData
**
** Description:
**	Reads a SqlLongChar data value from the current input message.
**	Only a single stream data value may be active at a given time.
**	Attempting to read another stream data value will result in
**	the closure of the preceding stream.
**
**	A SqlLongChar data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by zero or more segments
**	which are then followed by an end-of-segments indicator.  Each
**	segment is composed of a two byte integer length (non-zero) 
**	followed by a character byte array of the indicated length.  
**	The end-of-segments indicator is a two byte integer with value
**	zero.
**
**	Only the SQL data/NULL indicator is read by this method.  If
**	the SqlLongChar value is non-NULL, an InputStream is provided
**	for reading the segmented data stream.  The InputStream also
**	implements the SqlStream StreamSource interface for providing
**	notification of stream closure events.
**
** Input:
**	None.
**
** Output:
**	value	SQL data value.
**
** Returns:
**	void.
**
** History:
**	22-Sep-03 (gordy)
**	    Created.
**	15-Nov-06 (gordy)
**	    Added readByteStream() to access stream.
*/

public void
readSqlData( SqlLongChar value )
    throws SQLException
{
    if ( readSqlDataIndicator() )	// Read data indicator byte.
	value.set( readByteStream() );
    else
        value.setNull();		// NULL data value.
    return;
} // readSqlData


/*
** Name: readSqlData
**
** Description:
**	Reads a SqlLongCharCache data value from the current input message.
**
**	A SqlLongChar data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by zero or more segments
**	which are then followed by an end-of-segments indicator.  Each
**	segment is composed of a two byte integer length (non-zero) 
**	followed by a character byte array of the indicated length.  
**	The end-of-segments indicator is a two byte integer with value
**	zero.
**
**	Only the SQL data/NULL indicator is read by this method.  If
**	the SqlLongChar value is non-NULL, an InputStream is provided
**	for reading the segmented data stream.
**
** Input:
**	None.
**
** Output:
**	value	SQL data value.
**
** Returns:
**	void.
**
** History:
**	26-Feb-07 (gordy)
**	    Created.
*/

public void
readSqlData( SqlLongCharCache value )
    throws SQLException
{
    if ( readSqlDataIndicator() )	// Read data indicator byte.
	value.set( readByteStream() );
    else
        value.setNull();		// NULL data value.
    return;
} // readSqlData


/*
** Name: readSqlData
**
** Description:
**	Reads a SqlLongNChar data value from the current input message.
**	Only a single stream data value may be active at a given time.
**	Attempting to read another stream data value will result in
**	the closure of the preceding stream.
**
**	A SqlLongNChar data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by zero or more segments
**	which are then followed by an end-of-segments indicator.  Each
**	segment is composed of a two byte integer length (non-zero) 
**	followed by a UCS2 (two byte integer) array of the indicated 
**	length.  The end-of-segments indicator is a two byte integer 
**	with value zero.
**
**	Only the SQL data/NULL indicator is read by this method.  If
**	the SqlLongChar value is non-NULL, an Reader is provided for
**	reading the segmented data stream.  The Reader also implements 
**	the SqlStream StreamSource interface for providing notification 
**	of stream closure events.
**
** Input:
**	None.
**
** Output:
**	value	SQL data value.
**
** Returns:
**	void.
**
** History:
**	22-Sep-03 (gordy)
**	    Created.
**	15-Nov-06 (gordy)
**	    Added readUCS2Reader() to access stream.
*/

public void
readSqlData( SqlLongNChar value )
    throws SQLException
{
    if ( readSqlDataIndicator() )	// Read data indicator byte.
	value.set( readUCS2Reader() );
    else
        value.setNull();		// NULL data value.
    return;
} // readSqlData


/*
** Name: readSqlData
**
** Description:
**	Reads a SqlLongNCharCache data value from the current input message.
**
**	A SqlLongNChar data value is composed of a data indicator byte 
**	which, if not NULL, may be followed by zero or more segments
**	which are then followed by an end-of-segments indicator.  Each
**	segment is composed of a two byte integer length (non-zero) 
**	followed by a UCS2 (two byte integer) array of the indicated 
**	length.  The end-of-segments indicator is a two byte integer 
**	with value zero.
**
**	Only the SQL data/NULL indicator is read by this method.  If
**	the SqlLongChar value is non-NULL, an Reader is provided for
**	reading the segmented data stream.
**
** Input:
**	None.
**
** Output:
**	value	SQL data value.
**
** Returns:
**	void.
**
** History:
**	26-Feb-07 (gordy)
**	    Created.
*/

public void
readSqlData( SqlLongNCharCache value )
    throws SQLException
{
    if ( readSqlDataIndicator() )	// Read data indicator byte.
	value.set( readUCS2Reader() );
    else
        value.setNull();		// NULL data value.
    return;
} // readSqlData


/*
** Name: readSqlDataIndicator
**
** Description:
**	Reads a single byte (the SQL data/NULL indicator)
**	from the current message and returns True if a data 
**	value is indicated as being present (indicator is 
**	non-zero, or not-NULL).
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean		True if indicator is non-zero.
**
** History:
**	22-Sep-03 (gordy)
**	    Created.
*/

private boolean
readSqlDataIndicator()
    throws SQLException
{
    return( readByte() != 0 );
} // readSqlNullIndicator


/*
** Name: need
**
** Description:
**	Ensures that the required amount of data is available
**	in the input buffer.  A continuation message may be
**	loaded if required to satisfy the request.  Atomic
**	values require that the entire value be available in
**	in a message (no splitting).  Non-atomic values may
**	split and only require a portion of the value in the
**	message.  
**
** Input:
**	length	Amount of data required for request.
**	atomic	True if value is atomic, false otherwise.
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

private void
need( int length, boolean atomic )
    throws SQLException
{
    byte msg_id = in_msg_id;

    /*
    ** Nothing to do if data is in current message.
    */
    while( length > in_msg_len )
    {
	/*
	** Atomic values are not split across continued messages
	** and must be completely available or no portion at all.
	** Non-atomic values are satisfied if any portion is
	** available.
	*/
	if ( in_msg_len > 0 )
	    if ( ! atomic )
		break;	    // Data is availabe for non-atomic
	    else
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( title + ": atomic value split (" + 
				 in_msg_len + "," + length + ")" );
		disconnect();
		throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	    }

	/*
	** See if any more data is available.
	*/
	if ( ! moreData() )
	{
	    if ( trace.enabled( 1 ) )  
		trace.write( title + ": unexpected end-of-data" );
	    disconnect();
	    throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	}
    }

    return;
} // need


/*
** Name: ByteSegIS
**
** Description:
**	Class for converting segmented byte input into a stream.
**
**	This class extends InputStream to provide desegmentation 
**	of a BLOB (binary or character), bufferring the stream 
**	as needed.
**
**  Methods Overriden
**
**	read			Read bytes from segmented stream.
**	skip			Skip bytes in segmented stream.
**	available		Number of bytes in current segment.
**	close			Close (flush) segmented stream.
**
**  Public Methods
**
**	begin			Begin reading new BLOB.
**	addStreamListener	Register SQL Stream event listener.
**
**  Private Methods
**
**	next			Discard segment and begin next.
**	endStream		Set state at end of stream.
**
**  Private Data
**
**	listener		SQL Stream event listener.
**	stream			The SQL Stream.
**	msg_in			Associated MsgIn.
**	msg_id			Current message ID.
**	end_of_data		End-of-data received?
**	seg_len			Remaining data in current segment.
**	ba			Temporary storage.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
**	17-Nov-99 (gordy)
**	    Made class nested (inner) and removed constructor.
**	25-Jan-00 (gordy)
**	    Added tracing.
**	22-Sep-03 (gordy)
**	    Added support for SqlStream stream events: listener,
**	    stream, addStreamListener(), and endStream().  Removed
**	    begin() method as now represents a single BLOB value.
**	15-Nov-06 (gordy)
**	    Added begin() to permit re-use.
*/

private static class
ByteSegIS
    extends InputStream
    implements SqlStream.StreamSource
{

    private String	title;			// Class title for tracing.
    private Trace	trace;			// Tracing.
    
    private MsgIn	msg_in;			// Db connection input.
    private byte	msg_id = -1;		// Current message ID.
    private boolean	end_of_data = true;	// Segmented stream closed.
    private short	seg_len = 0;		// Current segment length
    private byte	ba[] = new byte[ 1 ];	// Temporary storage.

    private SqlStream.StreamListener	listener = null;
    private SqlStream			stream = null;
    

/*
** Name: ByteSegIS
**
** Description:
**	Class constructor.
**
** Input:
**	msg_in	    Db connection input.
**	trace	    Tracing output.
**	msg_id	    Message ID.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	21-Apr-00 (gordy)
**	    Created.
**	22-Sep-03 (gordy)
**	    Added msg_id parameter from former begin() method.
*/

public
ByteSegIS( MsgIn msg_in, Trace trace, byte msg_id )
{
    this.msg_in = msg_in;
    this.trace = trace;
    this.msg_id = msg_id;
    end_of_data = false;
    title = "ByteSegIS[" + msg_in.connID() + "]";
    if ( trace.enabled( 3 ) )  trace.write( title + ": new BLOB" );
} // ByteSegIS


/*
** Name: begin
**
** Description:
**	Initialize stream to read new BLOB.
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
**	15-Nov-06 (gordy)
**	    created.
*/

public void
begin( byte msg_id )
{
    this.msg_id = msg_id;
    end_of_data = false;
    seg_len = 0;
    listener = null;
    stream = null;

    if ( trace.enabled( 3 ) )  trace.write( title + ": begin BLOB" );
    return;
} // reset


/*
** Name: addStreamListener
**
** Description:
**	Add a StreamListener.  Only a single listener is supported.
**
** Input:
**	listener	SqlStream listener.
**	stream		SqlStream associated with source/listener.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	22-Sep-03 (gordy)
**	    Created.
*/

public void 
addStreamListener( SqlStream.StreamListener listener, SqlStream stream )
{
    this.listener = listener;
    this.stream = stream;
    return;
} // addStreamListener


/*
** Name: read
**
** Description:
**	Read a single byte from the segmented stream.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Byte read, -1 at end-of-data.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
*/

public int
read()
    throws IOException
{
    int len = read( ba, 0, 1 );
    return( len == 1 ? (int)ba[ 0 ] & 0xff : -1 );
} // read


/*
** Name: read
**
** Description:
**	Read a byte sub-array from the segment stream.
**
** Input:
**	offset	Offset in byte array.
**	length	Number of bytes to read.
**
** Output:
**	bytes	Byte array.
**
** Returns:
**	int	Number of bytes read, or -1 if end-of-data.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
*/

public int
read( byte bytes[], int offset, int length )
    throws IOException
{
    int len, total = 0;

    if ( trace.enabled( 4 ) )  trace.write( title + ".read(" + length + ")" );

    try
    {
	while( length > 0  &&  ! end_of_data )
	{
	    if ( seg_len <= 0  &&  ! next() )  break;
	    len = Math.min( length, seg_len );

	    len = msg_in.readBytes( bytes, offset, len );

	    if ( trace.enabled( 5 ) )
		trace.write( title + ": read " + len + " bytes" );
	    offset += len;
	    length -= len;
	    seg_len -= (short)len;
	    total += len;
	}
    }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write( title + ": error reading data" );
	    SqlExFactory.trace(ex, trace );
	}

	throw new IOException( ex.getMessage() );
    }

    if ( trace.enabled( 4 ) )  trace.write( title + ".read: " + total +
					    (end_of_data ? " [EOD]" : "") );
    return( total > 0 ? total : (end_of_data ? -1 : 0) );
} // read


/*
** Name: skip
**
** Description:
**	Discard bytes from segmented stream.
**
** Input:
**	length	    Number of bytes to skip.
**
** Output:
**	None.
**
** Returns:
**	long	    Number of bytes skipped.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
*/

public long
skip( long length )
    throws IOException
{
    long    total = 0;
    int	    len;

    if ( trace.enabled( 4 ) )  trace.write( title + ".skip(" + length + ")" );

    try
    {
	while( length > 0  &&  ! end_of_data )
	{
	    if ( seg_len <= 0  &&  ! next() )  break;
	    if ( (len = seg_len) > length )  len = (int)length;

	    len = msg_in.skip( len );

	    if ( trace.enabled( 5 ) )
		trace.write( title + ": skip " + len + " bytes" );
	    length -= len;
	    seg_len -= (short)len;
	    total += len;
	}
    }
    catch( SQLException ex )
    { 
	if ( trace.enabled( 1 ) )
	{
	    trace.write( title + ": error skipping data" );
	    SqlExFactory.trace(ex, trace );
	}

	throw new IOException( ex.getMessage() ); 
    }

    if ( trace.enabled( 4 ) )  trace.write( title + ".skip: " + total +
					    (end_of_data ? " [EOD]" : "") );
    return( total );
} // skip


/*
** Name: available
**
** Description:
**	Returns the number of bytes remaining in the current segment.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Number of bytes available.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
*/

public int
available()
    throws IOException
{
    return( seg_len );
} // available


/*
** Name: close
**
** Description:
**	Close the segmented stream.  If the end-of-data has not
**	yet been reached, the stream is read and discarded until
**	end-of-data is reached.
**
** Input:
**	None.
**
** Ouput:
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
close()
    throws IOException
{
    while( ! end_of_data )  next();
    return;
} // close


/*
** Name: next
**
** Description:
**	Discard current segment and begin next segment.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    Next segment available (not end-of-data).
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
**	12-Apr-01 (gordy)
**	    Pass tracing to exception.
*/

private boolean
next()
    throws IOException
{
    try
    {
	while( seg_len > 0 )  
	{
	    if ( trace.enabled( 4 ) )
		trace.write( title + ": skipping " + seg_len );
	    seg_len -= (short)msg_in.skip( seg_len );
	}

	if ( ! msg_in.moreData() )
	    if ( msg_in.receive() != msg_id )
	    {
		if ( trace.enabled( 3 ) )  
		    trace.write( title + ": truncated!" );
		endStream();
		throw new IOException( "BLOB data stream truncated!" );
	    }

	if ( (seg_len = msg_in.readShort()) == 0 )
	{
	    if ( trace.enabled( 3 ) )  trace.write( title + ": end BLOB" );
	    endStream();
	}
	else  
	{
	    if ( trace.enabled( 4 ) )
		trace.write( title + ": BLOB segment length " + seg_len );
	}
    }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write( title + ": error reading next segment" );
	    SqlExFactory.trace(ex, trace );
	}

	throw new IOException( ex.getMessage() ); 
    }

    return( ! end_of_data );
} // next


/*
** Name: endStream
**
** Description:
**	Set state when end of stream is reached.
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
**	22-Sep-03 (gordy)
**	    Created.
*/

private void
endStream()
{
    if ( ! end_of_data )
    {
	/*
	** Set inactive state.
	*/
	end_of_data = true;
	seg_len = 0;
	msg_id = -1;
    
	/*
	** If a listener has been registered, provide
	** notification of the stream closure event.
	*/
	if ( listener != null )  
	{
	    if ( trace.enabled( 5 ) )  
		trace.write( title + ": stream closure event notification" );
	    listener.streamClosed( stream );
	}
    }
    return;
} // endStream


} // class ByteSegIS


/*
** Name: Ucs2SegRdr
**
** Description:
**	Class for converting segmented UCS2 input into a stream.
**
**	This class extends Reader to provide desegmentation of a 
**	CLOB (UCS2), bufferring the stream as needed.
**
**  Methods Overridden
**
**	read			Read bytes from segmented stream.
**	skip			Skip bytes in segmented stream.
**	ready			Is there data buffered.
**	close			Close (flush) segmented stream.
**
**  Public Methods
**
**	begin			Begin reading new CLOB.
**	addStreamListener	Register SQL Stream event listener.
**
**  Private Methods
**
**	next			Discard segment and begin next.
**	endStream		Set state at end of stream.
**
**  Private Data
**
**	listener		SQL Stream event listener.
**	stream			The SQL Stream.
**	msg_in			Message Layer input.
**	msg_id			Current message ID.
**	end_of_data		End-of-data received?
**	seg_len			Remaining data in current segment.
**	ca			Temporary storage.
**
** History:
**	10-May-01 (gordy)
**	    Created.
**	22-Sep-03 (gordy)
**	    Added support for SqlStream stream events: listener,
**	    stream, addStreamListener(), and endStream().  Removed
**	    begin() method as now represents a single BLOB value.
**	15-Nov-06 (gordy)
**	    Added begin() to permit re-use.
*/

private static class
Ucs2SegRdr
    extends Reader
    implements SqlStream.StreamSource
{

    private String	title;			// Class title for tracing.
    private Trace	trace;			// Tracing.
    
    private MsgIn	msg_in;			// Message Layer input.
    private byte	msg_id = -1;		// Current message ID.
    private boolean	end_of_data = true;	// Segmented stream closed.
    private short	seg_len = 0;		// Current segment length
    private char	ca[] = new char[ 1 ];	// Temporary storage.

    private SqlStream.StreamListener	listener = null;
    private SqlStream			stream = null;


/*
** Name: Ucs2SegRdr
**
** Description:
**	Class constructor.
**
** Input:
**	msg_in	    Message layer input.
**	trace	    Tracing output.
**	msg_id	    Message ID.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	10-May-01 (gordy)
**	    Created.
**	22-Sep-03 (gordy)
**	    Added msg_id parameter from former begin() method.
*/

public
Ucs2SegRdr( MsgIn msg_in, Trace trace, byte msg_id )
{
    this.msg_in = msg_in;
    this.trace = trace;
    this.msg_id = msg_id;
    end_of_data = false;
    title = "Ucs2SegRdr[" + msg_in.connID() + "]";
    if ( trace.enabled( 3 ) )  trace.write( title + " new CLOB" );
} // Ucs2SegRdr


/*
** Name: begin
**
** Description:
**	Initialize reader to read new CLOB.
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
**	15-Nov-06 (gordy)
**	    created.
*/

public void
begin( byte msg_id )
{
    this.msg_id = msg_id;
    end_of_data = false;
    seg_len = 0;
    listener = null;
    stream = null;

    if ( trace.enabled( 3 ) )  trace.write( title + " begin CLOB" );
    return;
} // reset


/*
** Name: addStreamListener
**
** Description:
**	Add a StreamListener.  Only a single listener is supported.
**
** Input:
**	listener	SqlStream listener.
**	stream		SqlStream associated with source/listener.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	22-Sep-03 (gordy)
**	    Created.
*/

public void 
addStreamListener( SqlStream.StreamListener listener, SqlStream stream )
{
    this.listener = listener;
    this.stream = stream;
    return;
} // addStreamListener


/*
** Name: read
**
** Description:
**	Read a single character from the segmented stream.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Character read, -1 at end-of-data.
**
** History:
**	10-May-01 (gordy)
**	    Created.
*/

public int
read()
    throws IOException
{
    int len = read( ca, 0, 1 );
    return( len == 1 ? (int)ca[ 0 ] & 0xffff : -1 );
} // read


/*
** Name: read
**
** Description:
**	Read a character array from the segment stream.
**
** Input:
**	None
**
** Output:
**	cbuf	Character array.
**
** Returns:
**	int	Number of characters read, or -1 if end-of-data.
**
** History:
**	10-May-01 (gordy)
**	    Created.
*/

public int
read( char cbuf[] )
    throws IOException
{
    return( read( cbuf, 0, cbuf.length ) );
}


/*
** Name: read
**
** Description:
**	Read a character sub-array from the segment stream.
**
** Input:
**	offset	Offset in character array.
**	length	Number of characters to read.
**
** Output:
**	cbuf	Character array.
**
** Returns:
**	int	Number of characters read, or -1 if end-of-data.
**
** History:
**	10-May-01 (gordy)
**	    Created.
*/

public int
read( char cbuf[], int offset, int length )
    throws IOException
{
    int len, total = 0;

    if ( trace.enabled( 4 ) )  trace.write( title + ".read(" + length + ")" );

    try
    {
	while( length > 0  &&  ! end_of_data )
	{
	    if ( seg_len <= 0  &&  ! next() )  break;
	    len = Math.min( length, seg_len );

	    for( int end = offset + len; offset < end; offset++ )
		cbuf[ offset ] = (char)msg_in.readShort();

	    if ( trace.enabled( 5 ) )
		trace.write( title + ": read " + len + " UCS2 chars" );
	    seg_len -= (short)len;
	    length -= len;
	    total += len;
	}
    }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write( title + ": error reading data" );
	    SqlExFactory.trace(ex, trace );
	}

	throw new IOException( ex.getMessage() ); 
    }

    if ( trace.enabled( 4 ) )  trace.write( title + ".read: " + total + 
					    (end_of_data ? " [EOD]" : "") );
    return( total > 0 ? total : (end_of_data ? -1 : 0) );
} // read


/*
** Name: skip
**
** Description:
**	Discard characters from segmented stream.
**
** Input:
**	length	    Number of characters to skip.
**
** Output:
**	None.
**
** Returns:
**	long	    Number of characters skipped.
**
** History:
**	10-May-01 (gordy)
**	    Created.
*/

public long
skip( long length )
    throws IOException
{
    long    total = 0;
    int	    len;

    if ( trace.enabled( 4 ) )  trace.write( title + ".skip(" + length + ")" );

    try
    {
	while( length > 0  &&  ! end_of_data )
	{
	    if ( seg_len <= 0  &&  ! next() )  break;
	    if ( (len = seg_len) > length )  len = (int)length;

	    len = msg_in.skip( len * 2 ) / 2;

	    if ( trace.enabled( 5 ) )
		trace.write( title + ": skip " + len + " UCS2 chars" );
	    seg_len -= (short)len;
	    length -= len;
	    total += len;
	}
    }
    catch( SQLException ex )
    { 
	if ( trace.enabled( 1 ) )
	{
	    trace.write( title + ": error skipping data" );
	    SqlExFactory.trace(ex, trace );
	}

	throw new IOException( ex.getMessage() ); 
    }

    if ( trace.enabled( 4 ) )  trace.write( title + ".skip: " + total +
					    (end_of_data ? " [EOD]" : "") );
    return( total );
} // skip


/*
** Name: ready
**
** Description:
**	Is data available.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if data is available, false otherwise..
**
** History:
**	10-May-01 (gordy)
**	    Created.
*/

public boolean
ready()
    throws IOException
{
    return( seg_len > 0 );
} // ready


/*
** Name: close
**
** Description:
**	Close the segmented stream.  If the end-of-data has not
**	yet been reached, the stream is read and discarded until
**	end-of-data is reached.
**
** Input:
**	None.
**
** Ouput:
**	None.
**
** Returns:
**	void.
**
** History:
**	10-May-01 (gordy)
**	    Created.
*/

public void
close()
    throws IOException
{
    while( ! end_of_data )  next();
    return;
} // close


/*
** Name: next
**
** Description:
**	Discard current segment and begin next segment.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    Next segment available (not end-of-data).
**
** History:
**	10-May-01 (gordy)
**	    Created.
*/

private boolean
next()
    throws IOException
{
    try
    {
	while( seg_len > 0 )  
	{
	    if ( trace.enabled( 4 ) )
		trace.write( title + ": skipping " + seg_len );
	    seg_len -= (short)msg_in.skip( seg_len * 2 ) / 2;
	}

	if ( ! msg_in.moreData() )
	    if ( msg_in.receive() != msg_id )
	    {
		if ( trace.enabled( 3 ) )  
		    trace.write( title + ": truncated!" );
		endStream();
		throw new IOException( "CLOB data stream truncated!" );
	    }

	if ( (seg_len = msg_in.readShort()) == 0 )
	{
	    if ( trace.enabled( 3 ) )  trace.write( title + ": end CLOB" );
	    endStream();
	}
	else
	{
	    if ( trace.enabled( 4 ) )
		trace.write( title + ": CLOB segment length " + seg_len );
	}
    }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write( title + ": error reading next segment" );
	    SqlExFactory.trace(ex, trace );
	}

	throw new IOException( ex.getMessage() ); 
    }

    return( ! end_of_data );
} // next


/*
** Name: endStream
**
** Description:
**	Set state when end of stream is reached.
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
**	22-Sep-03 (gordy)
**	    Created.
*/

private void
endStream()
{
    if ( ! end_of_data )
    {
	/*
	** Set inactive state.
	*/
	end_of_data = true;
	seg_len = 0;
	msg_id = -1;
    
	/*
	** If a listener has been registered, provide
	** notification of the stream closure event.
	*/
	if ( listener != null )
	{
	    if ( trace.enabled( 5 ) )  
		trace.write( title + ": stream closure event notification" );
	    listener.streamClosed( stream );
	}
    }
    return;
} // endStream


} // class Ucs2SegRdr


} // class MsgIn

