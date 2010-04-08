/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.io;

/*
** Name: DbConnIn.java
**
** Description:
**	EDBC Driver class DbConnIn providing the capability to
**	input message from the JDBC (and DBMS) server.
**
**	The DbConn* classes present a single unified class,
**	through inheritance from DbConnIo to DbConn, for
**	access to the JDBC server.  They are divided into
**	separate classes to group related functionality for
**	ease in management.
**
**  Classes:
**
**	DbConnIn	Database connection input.
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
*/

import	java.io.InputStream;
import	java.io.InputStreamReader;
import	java.io.Reader;
import	java.io.IOException;
import	ca.edbc.util.EdbcEx;
import	ca.edbc.util.Trace;
import	ca.edbc.util.IdMap;
import	ca.edbc.util.MsgConst;
import	ca.edbc.util.CharSet;


/*
** Name: DbConnIn
**
** Description:
**	Ingres JDBC Driver class providing methods for reading messages 
**	from a database connection.  This is an intermediate class of the 
**	DbConn* group of classes.  When reading messages, the caller must 
**	provide multi-threaded protection for each message.
**
**  Public Methods:
**
**	receive		Load the next input message.
**	moreData	More data in message?
**	skip		Skip bytes in input message.
**	readByte	Read a byte from input message.
**	readShort	Read a short value from input message.
**	readInt		Read an integer value from input message.
**	readFloat	Read a float value from input message.
**	readDouble	Read a double value from input message.
**	readBytes	Read a byte array from input message.
**	readString	Read a string value from input message.
**	readUCS2	Read a UCS2 string value from input message.
**	readBinary	Read a segmented binary stream.
**	readAscii	Read a segmented character stream.
**	readUnicode	Read a segmented UCS2 stream.
**
**  Protected Methods:
**
**	disconnect	Disconnect from the server.
**	close		Close the server connection.
**	setBuffSize	Set the I/O buffer size.
**
**  Private Data:
**
**	in		Input buffer.
**	in_msg_id	Current input message ID.
**	in_msg_len	Input message length.
**	in_msg_flg	Input message flags.
**	segIS		Segmented byte BLOB input stream.
**	segRdr		Segmented UCS2 BLOB reader.
**	char_buff	UCS2 Character buffer (readUCS2()).
**
**  Private Methods:
**
**	need		Make sure data is available in input message.
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
*/

class
DbConnIn
    extends DbConnOut
{

    private InBuff	in = null;
    private byte	in_msg_id = 0;
    private short	in_msg_len = 0;
    private byte	in_msg_flg = 0;
    private ByteSegIS	segIS = null;
    private Ucs2SegRdr	segRdr = null;
    private char	char_buff[] = new char[0];


/*
** Name: DbConn
**
** Description:
**	Class constructor.  Initializes the input buffer.  
**	Returns any response to the MSG layer parameter
**	info (array is 0 filled beyond the response).  The
**	array must be sufficiently long to hold any response
**
** Input:
**	host_id		Hostname and port.
**	info		Message layer parameter info.
**	info_len	Length of parameter info.
**
** Output:
**	info		Response info or zero filled.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	10-Sep-99 (gordy)
**	    Parameterized the initial TL connection data.
**	22-Sep-99 (gordy)
**	    Added character set/encoding.
**	17-Nov-99 (gordy)
**	    Extracted input functionality from DbConn.
**	 6-Sep-02 (gordy)
**	    Character encoding now encapsulated in CharSet class.
**	    Read character set during connection parameter processing
**	    and leave mapping to encoding for parameter validation.
**	    Trace character set and encoding mapping.
**	14-Jul-04 (rajus01) Bug # 112452; Issue #: 13244417
**	    Always use ASCII to convert character-set name to Unicode.
**	    It is a requirement that Server should send the character-set 
**	    name encoded in ASCII.
*/

protected
DbConnIn( String host_id, byte info[], short info_len )
    throws EdbcEx
{
    super( host_id, info, info_len );

    try 
    { 
	in = new InBuff( socket.getInputStream(), 
			 conn_id, 1 << JDBC_TL_PKT_MIN ); 
    }
    catch( Exception ex )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( title + ": error creating input buffer: " + 
			 ex.getMessage() );
	disconnect();
	throw EdbcEx.get( E_IO0001_CONNECT_ERR, ex );
    }

    String  cs = null;   // Character set connection parameter
    int	    size = JDBC_TL_PKT_MIN;
    short   tl_id;

    try
    {
	if ( trace.enabled( 2 ) )
	    trace.write( title + ": reading Connection Confirmation" );

	if ( (tl_id = in.receive()) != JDBC_TL_CC )
	{
	    if ( trace.enabled( 1 ) )  
		trace.write( title + ": invalid TL CC packet ID " + tl_id );
	    throw EdbcEx.get( E_IO0001_CONNECT_ERR );
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
		    throw EdbcEx.get( E_IO0001_CONNECT_ERR );

	    if ( in.avail() < param_len )
		throw EdbcEx.get( E_IO0001_CONNECT_ERR );

	    switch( param_id )
	    {
	    case JDBC_TL_CP_PROTO :
		if ( param_len == 1 )  
		    tl_proto = in.readByte();
		else
		    throw EdbcEx.get( E_IO0001_CONNECT_ERR );
		break;

	    case JDBC_TL_CP_SIZE :
		if ( param_len == 1 )  
		    size = in.readByte();
		else
		    throw EdbcEx.get( E_IO0001_CONNECT_ERR );
		break;

	    case JDBC_TL_CP_CHRSET :
		/*
		** !!!!! FIX-ME !!!!!
		** Bootstrapping character set will fail if DBMS
		** character set and driver default encoding are
		** not minimally compatible.
		*/
		{
		    byte ba[] = new byte[ param_len ];
		    
		    if ( in.readBytes( ba, 0, param_len ) == param_len )
	            {
			try
		        {
			   cs = new String( ba, "US-ASCII" );
			}
			catch( Exception ex )
		        {
			    if ( trace.enabled( 1 ) )  
		               trace.write( title + ": TL CP CHARSET: " + ex.toString() );
			    throw EdbcEx.get( E_IO0001_CONNECT_ERR ); 
			}
		    }
		    else
			throw EdbcEx.get( E_IO0001_CONNECT_ERR );
		}
		break;

	    case JDBC_TL_CP_MSG :
		if ( info == null  ||  param_len > info.length  ||
		     in.readBytes( info, 0, param_len ) != param_len )
		    throw EdbcEx.get( E_IO0001_CONNECT_ERR );		
		break;

	    default :
		if ( trace.enabled( 1 ) )  
		    trace.write( title + ": TL CR param ID " + param_id );
		throw EdbcEx.get( E_IO0001_CONNECT_ERR );
	    }
	}

	if ( in.avail() > 0 )  throw EdbcEx.get( E_IO0001_CONNECT_ERR );

	/*
	** Validate message parameters.
	*/
	if ( tl_proto < JDBC_TL_PROTO_1  ||  tl_proto > JDBC_TL_DFLT_PROTO  ||
	     size < JDBC_TL_PKT_MIN  ||  size > JDBC_TL_PKT_MAX )
	{
	    if ( trace.enabled( 1 ) )
		trace.write( title + ": invalid TL parameter: protocol " + 
			  tl_proto + ", size " + size );
	    throw EdbcEx.get( E_IO0001_CONNECT_ERR );
	}

	try { char_set = CharSet.getCharSet( cs ); }
	catch( Exception ex )
	{
	    if ( trace.enabled( 1 ) )
		trace.write( title + ": incompatible character set: " + cs );
	    throw EdbcEx.get( E_IO0001_CONNECT_ERR );
	}
    }
    catch( EdbcEx ex )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( title + ": error negotiating parameters" );
	disconnect();
	throw ex;
    }

    /*
    ** We initialized with minimum buffer size, then requested
    ** maximum size for the connection.  If the server accepts
    ** anything more than minimum, adjust the buffers accordingly.
    */
    if ( size != JDBC_TL_PKT_MIN )  setBuffSize( 1 << size );

    if ( trace.enabled( 3 ) )
    {
	trace.write( title + ": TL connection parameters negotiated" );
	trace.write( "    TL protocol level : " + tl_proto );
	trace.write( "    TL buffer size    : " + (1 << size) );
	trace.write( "    Character encoding: " + cs + " => " + char_set );
    }
} // DbConnIn


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
*/

protected void
close()
{
    super.close();

    try 
    { 
	while( in.receive() != JDBC_TL_DC ); 
	if ( trace.enabled( 2 ) )
	    trace.write( title + ": Disconnect Confirm received" );
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
    in.setBuffSize( size );
    super.setBuffSize( size );
    return;
} // setBuffSize


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
*/

public byte
receive()
    throws EdbcEx
{
    int msg_id;

    if ( trace.enabled( 3 ) )  trace.write( title + ": reading next message" );

    /*
    ** Check to see if message header is in the input
    ** buffer (handles message concatenation).
    */
    while( in.avail() < 8 )
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
	    disconnect();
	    throw EdbcEx.get( E_IO0002_PROTOCOL_ERR );
	}

	try { tl_id = in.receive(); }
	catch( EdbcEx ex )
	{
	    disconnect();
	    throw ex;
	}

	if ( tl_id != JDBC_TL_DT )
	{
	    if ( trace.enabled( 1 ) )  
		trace.write( title + ": invalid TL packet ID 0x" +
			     Integer.toHexString( tl_id ) );
	    disconnect();
	    throw EdbcEx.get( E_IO0002_PROTOCOL_ERR );
	}
    }

    /*
    ** Read and validate message header.
    */
    if ( (msg_id = in.readInt()) != JDBC_MSG_ID )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( title + ": invalid header ID 0x" +
			 Integer.toHexString( msg_id ) );
	disconnect();
	throw EdbcEx.get( E_IO0002_PROTOCOL_ERR );
    }

    in_msg_len = in.readShort();
    in_msg_id = in.readByte();
    in_msg_flg = in.readByte();

    if ( trace.enabled( 2 ) )  
	trace.write( title + ": received message " + 
		     IdMap.map( in_msg_id, MsgConst.msgMap ) + 
		     " length " + in_msg_len + 
		     ((in_msg_flg & JDBC_MSG_EOD) == 0 ? "" : " EOD") +
		     ((in_msg_flg & JDBC_MSG_EOG) == 0 ? "" : " EOG") );

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
    throws EdbcEx
{
    byte msg_id = in_msg_id;

    while( in_msg_len <= 0  &&  (in_msg_flg & JDBC_MSG_EOD) == 0 )
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
	    throw EdbcEx.get( E_IO0002_PROTOCOL_ERR );
	}
    }

    return( in_msg_len > 0 );
} // moreData


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
    throws EdbcEx
{
    int total = 0;

    if ( trace.enabled( 3 ) )
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
    throws EdbcEx
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
    throws EdbcEx
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
    throws EdbcEx
{
    need( 4, true );
    in_msg_len -= 4;
    return( in.readInt() );
} // readInt

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
    throws EdbcEx
{
    need( JDBC_TL_F4_LEN, true );
    in_msg_len -= JDBC_TL_F4_LEN;
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
    throws EdbcEx
{
    need( JDBC_TL_F8_LEN, true );
    in_msg_len -= JDBC_TL_F8_LEN;
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
**	byte[]	    Input byte array.
**
** History:
**	18-Apr-01 (gordy)
**	    Created.
*/

public byte[]
readBytes()
    throws EdbcEx
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
**	byte[]	    Input byte array.
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
    throws EdbcEx
{
    byte    bytes[] = new byte[ length ];
    readBytes( bytes, 0, length );
    return( bytes );
} // readBytes

/*
** Name: readBytes
**
** Description:
**	Reads bytes from the current input message.
**
** Input:
**	bytes	    Byte array.
**	offset	    Offset in byte array.
**	length	    Number of bytes.
**
** Output:
**	None.
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
    throws EdbcEx
{
    int len, total = 0;

    while( length > 0 )
    {
	need( length, false );
	len = Math.min( in_msg_len, length );
	len = in.readBytes( bytes, offset, len );
	offset += len;
	length -= len;
	in_msg_len -= len;
	total += len;
    }

    return( total );
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
**	String	    Input string value.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
*/

public String
readString()
    throws EdbcEx
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
**	String	    Input string value.
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
    throws EdbcEx
{
    String  str;

    if ( length <= in_msg_len )	    // Is entire string available?
	if ( length <= 0 )
	    str = new String();
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
	catch( Exception e )
	{
	    // Should not happen!
	    throw new EdbcEx( title + ": character encoding failed" );
	}
    }

    return( str );
} // readString


/*
** Name: readUCS2
**
** Description:
**	Reads a UCS2 string from the current input message.  The
**	length of the string is read from the input message.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	    Input UCS2 value.
**
** History:
**	10-May-01 (gordy)
**	    Created.
*/

public String
readUCS2()
    throws EdbcEx
{
    return( readUCS2( readShort() ) );
} // readUCS2


/*
** Name: readUCS2
**
** Description:
**	Reads a UCS2 string from the current input message.
**
** Input:
**	length	    Length of the UCS2 string to read.
**
** Output:
**	None.
**
** Returns:
**	String	    Input UCS2 value.
**
** History:
**	10-May-01 (gordy)
**	    Created.
*/

public String
readUCS2( int length )
    throws EdbcEx
{
    if ( length > char_buff.length )  char_buff = new char[ length ];
    length = readUCS2( char_buff, 0, length );
    return( new String( char_buff, 0, length ) );
} // readUCS2


/*
** Name: readUCS2
**
** Description:
**	Reads UCS2 characters from the current input message.
**
** Input:
**	chars	    Character array.
**	offset	    Offset in character array.
**	length	    Number of characters.
**
** Output:
**	None.
**
** Returns:
**	int	    Number of characters actually read.
**
** History:
**	10-May-01 (gordy)
**	    Created.
*/

public int
readUCS2( char chars[], int offset, int length )
    throws EdbcEx
{
    for( int end = offset + length; offset < end; offset++ )
	chars[ offset ] = (char)readShort();
    return( length );
} // readUCS2


/*
** Name: readBinary
**
** Description:
**	Unlike the other read methods, this method does not actually
**	read data from the input stream.  Instead, the InputStream
**	instance returned may be used to read segmented byte input
**	as a byte stream.  The InputStream instance returned should
**	only be used when the data in the input stream is formatted
**	for byte stream access (the data is segmented with a leading
**	two byte length indicator on each segment and end-of-data is
**	indicated by a zero length segment).  No other read method
**	should be called until the InputStream has been closed.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	InputStream	InputStream instance.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
*/

public InputStream
readBinary()
{
    if ( segIS == null )  segIS = new ByteSegIS( this, trace );
    segIS.begin( in_msg_id );
    return( segIS );
} // readBinary


/*
** Name: readAscii
**
** Description:
**	Unlike the other read methods, this method does not actually
**	read data from the input stream.  Instead, the Reader instance
**	returned may be used to read segmented byte input as a Unicode
**	character stream.  The Reader instance returned should only be
**	used when the data in the input stream is formatted for byte 
**	stream access (the data is segmented with a leading two byte 
**	length indicator on each segment and end-of-data is indicated 
**	by a zero length segment).  No other read method should be 
**	called until the Reader has been closed.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Reader	Reader instance.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
**	 6-Sep-02 (gordy)
**	    Moved character encoding conversion to CharSet class.
*/

public Reader
readAscii()
    throws EdbcEx
{
    InputStreamReader isr;

    if ( segIS == null )  segIS = new ByteSegIS( this, trace );
    segIS.begin( in_msg_id );

    try { isr = char_set.getISR( segIS ); }
    catch( Exception e )
    {
        // Should not happen!
        throw new EdbcEx( title + ": character encoding failed" );
    }

    return( isr );
} // readAscii


/*
** Name: readUnicode
**
** Description:
**	Unlike the other read methods, this method does not actually
**	read data from the input stream.  Instead, the Reader instance
**	returned may be used to read segmented UCS2 input as a Unicode
**	character stream.  The Reader instance returned should only be
**	used when the data in the input stream is formatted for UCS2 
**	stream access (the data is segmented with a leading two byte 
**	length indicator on each segment and end-of-data is indicated 
**	by a zero length segment).  No other read method should be 
**	called until the Reader has been closed.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Reader	Reader instance.
**
** History:
**	10-May-01 (gordy)
**	    Created.
*/

public Reader
readUnicode()
    throws EdbcEx
{
    if ( segRdr == null )  segRdr = new Ucs2SegRdr( this, trace );
    segRdr.begin( in_msg_id );
    return( segRdr );
} // readUnicode


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
    throws EdbcEx
{
    byte msg_id = in_msg_id;

    if ( trace.enabled( 5 ) )
	trace.write( title + ": need " + length + 
		     (atomic ? " atomic" : "") + " bytes" );

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
		throw EdbcEx.get( E_IO0002_PROTOCOL_ERR );
	    }

	/*
	** See if any more data is available.
	*/
	if ( ! moreData() )
	{
	    if ( trace.enabled( 1 ) )  
		trace.write( title + ": unexpected end-of-data" );
	    disconnect();
	    throw EdbcEx.get( E_IO0002_PROTOCOL_ERR );
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
**	of a JDBC BLOB (binary or character), bufferring the stream 
**	as needed.
**
**	This class is designed for repeated usage.  The begin() 
**	method provides the means for a single class instance to 
**	process multiple serial BLOBs on the same connection.
**
**  Methods Overriden
**
**	read		Read bytes from segmented stream.
**	skip		Skip bytes in segmented stream.
**	available	Number of bytes in current segment.
**	close		Close (flush) segmented stream.
**
**  Public Methods
**
**	begin		Begin a new segmented stream.
**
**  Private Methods
**
**	next		Discard segment and begin next.
**
**  Private Data
**
**	dbc_in		Associated DbConnIn.
**	msg_id		Current message ID.
**	end_of_data	End-of-data received?
**	seg_len		Remaining data in current segment.
**	ba		Temporary storage.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
**	17-Nov-99 (gordy)
**	    Made class nested (inner) and removed constructor.
**	25-Jan-00 (gordy)
**	    Added tracing.
*/

private static class
ByteSegIS
    extends InputStream
{

    private String	title;			// Class title for tracing.
    private Trace	trace;			// Tracing.
    
    private DbConnIn	dbc_in;			// Db connection input.
    private byte	msg_id = -1;		// Current message ID.
    private boolean	end_of_data = true;	// Segmented stream closed.
    private short	seg_len = 0;		// Current segment length
    private byte	ba[] = new byte[ 1 ];	// Temporary storage.


/*
** Name: ByteSegIS
**
** Description:
**	Class constructor.
**
** Input:
**	dbc_in	    Db connection input.
**	trace	    Tracing output.
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
*/

public
ByteSegIS( DbConnIn dbc_in, Trace trace )
{
    title = "ByteSegIS[" + dbc_in.connID() + "]";
    this.dbc_in = dbc_in;
    this.trace = trace;
} // ByteSegIS


/*
** Name: begin
**
** Description:
**	Begin a new segmented stream.
**
** Input:
**	msg_id	    Message ID.
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
begin( byte msg_id )
{
    this.msg_id = msg_id;
    end_of_data = false;
    seg_len = 0;

    if ( trace.enabled( 3 ) )  trace.write( title + ": start of BLOB" );
    return;
} // begin


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

    if ( trace.enabled( 3 ) )
	trace.write( title + ": read " + length + " bytes" );

    try
    {
	while( length > 0  &&  ! end_of_data )
	{
	    if ( seg_len <= 0  &&  ! next() )  break;
	    len = Math.min( length, seg_len );

	    if ( trace.enabled( 4 ) )
		trace.write( title + ": reading " + len + " of " + seg_len );

	    len = dbc_in.readBytes( bytes, offset, len );
	    offset += len;
	    length -= len;
	    seg_len -= (short)len;
	    total += len;
	}
    }
    catch( Exception ex )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( title + ": error reading data: " + ex.getMessage() );
	throw new IOException( ex.getMessage() ); 
    }

    if ( trace.enabled( 4 ) )
	trace.write( title + ": " + total + " bytes read " +
		     (end_of_data ? " EOD" : "") );

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

    if ( trace.enabled( 3 ) )
	trace.write( title + ": skipping " + length + " bytes" );

    try
    {
	while( length > 0  &&  ! end_of_data )
	{
	    if ( seg_len <= 0  &&  ! next() )  break;
	    if ( (len = seg_len) > length )  len = (int)length;

	    if ( trace.enabled( 4 ) )
		trace.write( title + ": skipping " + len + " of " + seg_len );

	    len = dbc_in.skip( len );
	    length -= len;
	    seg_len -= (short)len;
	    total += len;
	}
    }
    catch( Exception ex )
    { 
	if ( trace.enabled( 1 ) )
	    trace.write( title + ": error skipping data: " + ex.getMessage() );
	throw new IOException( ex.getMessage() ); 
    }

    if ( trace.enabled( 4 ) )
	trace.write( title + ": skipped " + total + " bytes" );

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
    if ( trace.enabled( 3 ) )  trace.write( title + ": closing BLOB" );
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
    if ( trace.enabled( 4 ) )
	trace.write( title + ": read next segment" );

    try
    {
	while( seg_len > 0 )  
	    seg_len -= (short)dbc_in.skip( seg_len );

	if ( ! dbc_in.moreData() )
	    if ( dbc_in.receive() != msg_id )
	    {
		end_of_data = true;
		throw new IOException( "BLOB data stream truncated!" );
	    }

	if ( (seg_len = dbc_in.readShort()) == 0 )
	    end_of_data = true;
	
	if ( trace.enabled( 4 ) )
	    trace.write( title + ": new segment length " + seg_len );
    }
    catch( EdbcEx ex )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write( title + ": exception reading next segment" );
	    ex.trace( trace );
	}

	throw new IOException( ex.getMessage() ); 
    }

    return( ! end_of_data );
} // next

} // class ByteSegIS


/*
** Name: Ucs2SegRdr
**
** Description:
**	Class for converting segmented UCS2 input into a stream.
**
**	This class extends Reader to provide desegmentation of a 
**	JDBC BLOB (UCS2), bufferring the stream as needed.
**
**	This class is designed for repeated usage.  The begin() 
**	method provides the means for a single class instance to 
**	process multiple serial BLOBs on the same connection.
**
**  Methods Overriden
**
**	read		Read bytes from segmented stream.
**	skip		Skip bytes in segmented stream.
**	ready		Is there data buffered.
**	close		Close (flush) segmented stream.
**
**  Public Methods
**
**	begin		Begin a new segmented stream.
**
**  Private Methods
**
**	next		Discard segment and begin next.
**
**  Private Data
**
**	dbc_in		Associated DbConnIn.
**	msg_id		Current message ID.
**	end_of_data	End-of-data received?
**	seg_len		Remaining data in current segment.
**	ca		Temporary storage.
**
** History:
**	10-May-01 (gordy)
**	    Created.
*/

private static class
Ucs2SegRdr
    extends Reader
{

    private String	title;			// Class title for tracing.
    private Trace	trace;			// Tracing.
    
    private DbConnIn	dbc_in;			// Db connection input.
    private byte	msg_id = -1;		// Current message ID.
    private boolean	end_of_data = true;	// Segmented stream closed.
    private short	seg_len = 0;		// Current segment length
    private char	ca[] = new char[ 1 ];	// Temporary storage.


/*
** Name: Ucs2SegRdr
**
** Description:
**	Class constructor.
**
** Input:
**	dbc_in	    Db connection input.
**	trace	    Tracing output.
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
*/

public
Ucs2SegRdr( DbConnIn dbc_in, Trace trace )
{
    title = "Ucs2SegRdr[" + dbc_in.connID() + "]";
    this.dbc_in = dbc_in;
    this.trace = trace;
} // Ucs2SegRdr


/*
** Name: begin
**
** Description:
**	Begin a new segmented stream.
**
** Input:
**	msg_id	    Message ID.
**
** Output:
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
begin( byte msg_id )
{
    this.msg_id = msg_id;
    end_of_data = false;
    seg_len = 0;

    if ( trace.enabled( 3 ) )  trace.write( title + ": start of BLOB" );
    return;
} // begin


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

    if ( trace.enabled( 3 ) )
	trace.write( title + ": read " + length + " characters" );

    try
    {
	while( length > 0  &&  ! end_of_data )
	{
	    if ( seg_len <= 0  &&  ! next() )  break;
	    len = Math.min( length, seg_len );

	    if ( trace.enabled( 4 ) )
		trace.write( title + ": reading " + len + " of " + seg_len );

	    for( int end = offset + len; offset < end; offset++ )
		cbuf[ offset ] = (char)dbc_in.readShort();

	    seg_len -= (short)len;
	    length -= len;
	    total += len;
	}
    }
    catch( Exception ex )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( title + ": error reading data: " + ex.getMessage() );
	throw new IOException( ex.getMessage() ); 
    }

    if ( trace.enabled( 4 ) )
	trace.write( title + ": " + total + " characters read " +
		     (end_of_data ? " EOD" : "") );

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

    if ( trace.enabled( 3 ) )
	trace.write( title + ": skipping " + length + " characters" );

    try
    {
	while( length > 0  &&  ! end_of_data )
	{
	    if ( seg_len <= 0  &&  ! next() )  break;
	    if ( (len = seg_len) > length )  len = (int)length;

	    if ( trace.enabled( 4 ) )
		trace.write( title + ": skipping " + len + " of " + seg_len );

	    len = dbc_in.skip( len * 2 ) / 2;
	    seg_len -= (short)len;
	    length -= len;
	    total += len;
	}
    }
    catch( Exception ex )
    { 
	if ( trace.enabled( 1 ) )
	    trace.write( title + ": error skipping data: " + ex.getMessage() );
	throw new IOException( ex.getMessage() ); 
    }

    if ( trace.enabled( 4 ) )
	trace.write( title + ": skipped " + total + " characters" );

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
    if ( trace.enabled( 3 ) )  trace.write( title + ": closing BLOB" );
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
    if ( trace.enabled( 4 ) )
	trace.write( title + ": read next segment" );

    try
    {
	while( seg_len > 0 )  
	    seg_len -= (short)dbc_in.skip( seg_len * 2 ) / 2;

	if ( ! dbc_in.moreData() )
	    if ( dbc_in.receive() != msg_id )
	    {
		end_of_data = true;
		throw new IOException( "BLOB data stream truncated!" );
	    }

	if ( (seg_len = dbc_in.readShort()) == 0 )
	    end_of_data = true;
	
	if ( trace.enabled( 4 ) )
	    trace.write( title + ": new segment length " + seg_len );
    }
    catch( EdbcEx ex )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write( title + ": exception reading next segment" );
	    ex.trace( trace );
	}

	throw new IOException( ex.getMessage() ); 
    }

    return( ! end_of_data );
} // next

} // class Ucs2SegRdr

} // class DbConnIn

