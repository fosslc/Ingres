/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.io;

/*
** Name: DbConnOut.java
**
** Description:
**	EDBC Driver class DbConnOut providing capability to
**	output messages to the JDBC (and DBMS) server.
**
**	The DbConn* classes present a single unified class,
**	through inheritance from DbConnIo to DbConn, for
**	access to the JDBC server.  They are divided into
**	separate classes to group related functionality for
**	ease in management.
**
**  Classes:
**
**	DbConnOut
**	ByteSegOS
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
*/

import	java.io.InputStream;
import	java.io.Reader;
import	java.io.OutputStream;
import	java.io.OutputStreamWriter;
import	java.io.IOException;
import	ca.edbc.util.EdbcEx;
import	ca.edbc.util.Trace;
import	ca.edbc.util.IdMap;
import	ca.edbc.util.MsgConst;


/*
** Name: DbConnOut
**
** Description:
**	EDBC Driver class providing methods for writing messages on 
**	a database connection.  This is an intermediate class of the 
**	DbConn* group of classes.  When writing messages, the caller 
**	must provide multi-threaded protection for each message.
**
**  Public Methods:
**
**	begin		    Start a new output message.
**	getByteLength	    Returns length of encoded string.
**	write		    Append data to output message.
**	getUCS2ByteLength   Returns length of UCS2 encoded string.
**	writeUCS2	    Append char data as UCS2 to output message.
**	done		    Finish the current output message.
**
**  Protected Methods:
**
**	disconnect	Disconnect from the server.
**	close		Close the server connection.
**	setBuffSize	Set the I/O buffer size.
**
**  Private Data:
**
**	out		Output buffer.
**	segOS		BLOB output.
**	out_msg_id	Current output message ID.
**	out_hdr_pos	Message header buffer position.
**	out_hdr_len	Size of message header.
**	char_buff	Character buffer.
**
**  Private Methods:
**
**	split		Send current output message and continue message.
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
*/

class
DbConnOut
    extends DbConnIo
{

    private OutBuff	out = null;
    private ByteSegOS	segOS = null;

    private byte	out_msg_id = 0;
    private int		out_hdr_pos = 0;
    private int		out_hdr_len = 0;
    private char	char_buff[] = new char[ 8192 ];


/*
** Name: DbConnOut
**
** Description:
**	Class constructor.  Initializes the output buffer.  
**	Sends the Message layer parameter info byte array 
**	(if provided) and 0 fills the array (as output).
**
** Input:
**	host_id		Hostname and port.
**	info		Message layer parameter info, may be NULL.
**	info_len	Length of parameter info, may be 0.
**
** Output:
**	info		Filled with zeros.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	10-Sep-99 (gordy)
**	    Parameterized the initial TL connection data.
**	17-Nov-99 (gordy)
**	    Extracted output functionality from DbConn.
*/

protected
DbConnOut( String host_id, byte info[], short info_len )
    throws EdbcEx
{
    super( host_id );

    try 
    {
	out = new OutBuff( socket.getOutputStream(), 
			   conn_id, 1 << JDBC_TL_PKT_MIN );
    }
    catch( Exception ex )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( title + ": error creating output buffer: " + 
			 ex.getMessage() );
	disconnect();
	throw EdbcEx.get( E_IO0001_CONNECT_ERR, ex );
    }

    try
    {
	if ( trace.enabled( 2 ) )
	    trace.write( title + ": sending Connection Request" );

	/*
	** Send the Connection Request message with our desired
	** protocol level and buffer size.
	*/
	out.begin( JDBC_TL_CR, 3 );
	out.write( JDBC_TL_CP_PROTO );
	out.write( (byte)1 );
	out.write( JDBC_TL_DFLT_PROTO );
	out.write( JDBC_TL_CP_SIZE );
	out.write( (byte)1 );
	out.write( JDBC_TL_PKT_MAX );

	if ( info != null  &&  info_len > 0 )
	{
	    out.write( JDBC_TL_CP_MSG );

	    if ( info_len < 255 )
		out.write( (byte)info_len );
	    else
	    {
		out.write( (byte)255 );
		out.write( (short)info_len );
	    }
	    
	    out.write( info, 0, info_len );
	    for( int i = 0; i < info.length; i++ )  info[ i ] = 0;
	}

	out.flush();
    }
    catch( EdbcEx ex )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( title + ": error formatting/sending parameters" );
	disconnect();
	throw ex;
    }
} // DbConnOut


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
**	    Extracted output functionality from DbConn.
*/

protected void
close()
{
    try
    {
	out.clear();    // Clear partial messages from buffer.
	out.begin( JDBC_TL_DR, 0 );
	out.flush();
	if ( trace.enabled( 2 ) )
	    trace.write( title + ": Disconnect Request sent" );
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
*/

public void
begin( byte msg_id )
    throws EdbcEx
{
    if ( isClosed() )  throw EdbcEx.get( E_IO0004_CONNECTION_CLOSED );
    if ( out_hdr_pos > 0 )  done( false );	/* Complete current message */

    if ( trace.enabled( 3 ) )
	trace.write( title + ": beginning message " +
		     IdMap.map( msg_id, MsgConst.msgMap ) );

    /*
    ** Reserve space in output buffer for header.
    ** Note that we don't do message concatenation
    ** at this level.  We rely on messages being
    ** concatenated by the output buffer if not
    ** explicitly flushed.
    */
    try 
    { 
	out.begin( JDBC_TL_DT, 8 );	/* Begin new message */
	out_msg_id = msg_id;
	out_hdr_pos = out.position();
    
	out.write( JDBC_MSG_ID );	/* Eyecatcher */
	out.write( (short)0 );		/* Data length */
	out.write( msg_id );		/* Message ID */
	out.write( JDBC_MSG_EOD );	/* Flags */

	out_hdr_len = out.position() - out_hdr_pos;
    }
    catch( EdbcEx ex )
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
    throws EdbcEx
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
    throws EdbcEx
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
    throws EdbcEx
{
    if ( out.avail() < 4 )  split();
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
    throws EdbcEx
{
    if ( out.avail() < JDBC_TL_F4_LEN )  split();
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
    throws EdbcEx
{
    if ( out.avail() < JDBC_TL_F8_LEN )  split();
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
    throws EdbcEx
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
    throws EdbcEx
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
** Name: getByteLength
**
** Description:
**	Returns the length in bytes of the provided string 
**	when formatted for transmission to the Server.
**
** Input:
**	value	String
**
** Output:
**	None.
**
** Returns:
**	int	Formatted length in bytes.
**
** History:
**	 6-Sep-02 (gordy)
**	    Created.
*/

public int
getByteLength( String value )
{
    return( char_set.getByteLength( value ) );
} // getByteLength


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
    throws EdbcEx
{
    byte    ba[];

    try { ba = char_set.getBytes( value ); }
    catch( Exception ex )
    {
        // Should not happen!
        throw new EdbcEx( title + ": character encoding failed" );
    }

    write( ba );
    return;
} // write


/*
** Name: getByteLength
**
** Description:
**	Returns the length in bytes of the provided character
**	array when formatted for transmission to the Server.
**
** Input:
**	value	Character array
**
** Output:
**	None.
**
** Returns:
**	int	Formatted length in bytes.
**
** History:
**	 6-Sep-02 (gordy)
**	    Created.
*/

public int
getByteLength( char value[] )
{
    String  str = new String( value );
    return( char_set.getByteLength( str ) );
} // getByteLength


/*
** Name: write
**
** Description:
**	Append a character array to current output message.  If 
**	buffer overflow would occur, the message (and array) is 
**	split.  The character array is preceded by a two byte 
**	length indicator.
**
** Input:
**	value	    The character array to be appended.
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
**	 6-Sep-02 (gordy)
**	    Character encoding now encapsulated in CharSet class.
*/

public void
write( char value[] )
    throws EdbcEx
{
    String  str = new String( value );
    byte    ba[];

    try { ba = char_set.getBytes( str ); }
    catch( Exception ex )
    {
        // Should not happen!
        throw new EdbcEx( title + ": character encoding failed" );
    }

    write( ba );
    return;
} // write 


/*
** Name: write
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
**	 6-Sep-02 (gordy)
**	    Character encoding now encapsulated in CharSet class.
*/

public void
write( char value[], int offset, int length )
    throws EdbcEx
{
    String  str = new String( value, offset, length );
    byte    ba[];

    try { ba = char_set.getBytes( str ); }
    catch( Exception ex )
    {
        // Should not happen!
        throw new EdbcEx( title + ": character encoding failed" );
    }

    write( ba, 0, ba.length );
    return;
} // write


/*
** Name: getUCS2ByteLength
**
** Description:
**	Returns the length in bytes of the provided string
**	when formatted as UCS2 for transmission to the Server.
**
** Input:
**	value	String
**
** Output:
**	None.
**
** Returns:
**	int	Formatted length in bytes.
**
** History:
**	 6-Sep-02 (gordy)
**	    Created.
*/

public int
getUCS2ByteLength( String value )
{
    return( value.length() * 2 );
} // getUCS2ByteLength


/*
** Name: writeUCS2
**
** Description:
**	Append a string value to current output message.  If buffer
**	overflow would occur, the message (and string) is split.
**	The UCS2 array is preceded by a two byte length indicator.
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
**	 10-May-01 (gordy)
**	    Created.
*/

public void
writeUCS2( String value )
    throws EdbcEx
{
    int	length = value.length();
    write( (short)length );

    for( int offset = 0; offset < length; offset++ )
	write( (short)value.charAt( offset ) );
    return;
} // writeUCS2


/*
** Name: getUCS2ByteLength
**
** Description:
**	Returns the length in bytes of the provided character array
**	when formatted as UCS2 for transmission to the Server.
**
** Input:
**	value	Character array.
**
** Output:
**	None.
**
** Returns:
**	int	Formatted length in bytes.
**
** History:
**	 6-Sep-02 (gordy)
**	    Created.
*/

public int
getUCS2ByteLength( char value[] )
{
    return( value.length * 2 );
} // getUCS2ByteLength


/*
** Name: writeUCS2
**
** Description:
**	Append a character array to current output message.  If 
**	buffer overflow would occur, the message (and array) is 
**	split.  The UCS2 array is preceded by a two byte length 
**	indicator.
**
** Input:
**	value	    The character array to be appended.
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
writeUCS2( char value[] )
    throws EdbcEx
{
    write( (short)value.length );
    write( value, 0, value.length );
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
    throws EdbcEx
{
    for( int end = offset + length; offset < end; offset++ )
	write( (short)value[ offset ] );
    return;
} // writeUCS2


/*
** Name: writeUCS2
**
** Description:
**	Send an input Unicode stream to the server as a series of
**	segments.  Segments are sized to fit the amount of space
**	remaining in the buffer.  Additional segments will end
**	the current message and begin a new message with the
**	same message ID.  The last segment is left in the buffer
**	so the caller may append additional data.
**
**	A ByteSegOS (special local class) is used to write the
**	segment format of the bufferred output stream.
**
** Input:
**	in	    Input Unicode stream.
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
**	31-May-01 (gordy)
**	    Close request moved from finally() block to exception
**	    block so input stream won't be closed on first read().
**	    Increased buffering capacity.
*/

public void
writeUCS2( Reader in )
    throws EdbcEx
{
    byte    msg_id = out_msg_id;
    int	    len_pos, length;
    short   seg_len;

    if ( trace.enabled( 3 ) )  trace.write( title + ": starting CLOB" );

    do
    {
	/*
	** A CLOB segment requires a minimum of four bytes:
	** two for the length indicator and two bytes for
	** one character of data.
	*/
	if ( out.avail() < 4 )  
	{
	    done( false );	// Finish current message
	    begin( msg_id );	// Begin new message of same type.
	}

	/*
	** Write an initial length indicator of 0 (since
	** the segment has not been written yet) and save
	** the length indicator position for later updating.
	*/
	len_pos = out.position();
	seg_len = 0;
        out.write( seg_len );

	/*
	** Now try to write a segment which fills the current
	** message buffer.  Update length indicator with the
	** actual segment length written.
	*/
	while( (length = out.avail()) >= 2 )
	{
	    length = Math.min( length / 2, char_buff.length );

	    try
	    {
		length = in.read( char_buff, 0, length );
	    }
	    catch( Exception ex )
	    {
		try { in.close(); } catch( Exception ignore ) {}
		throw EdbcEx.get( E_IO0007_BLOB_IO, ex );
	    }

	    if ( length < 1 )  break;
	    seg_len += (short)length;

	    for( int i = 0; i < length; i++ )
		out.write( (short)char_buff[ i ] );
	}

	if ( seg_len > 0 )  
	{
	    out.write( len_pos, seg_len );
	    if ( trace.enabled( 4 ) )
		trace.write( title + ": wrote " + seg_len + " of " + seg_len );
	}
    } while( seg_len > 0 );

    if ( trace.enabled( 3 ) )  trace.write( title + ": closing CLOB" );
    try { in.close(); } catch( Exception ignore ) {}
    return;
} // writeUCS2


/*
** Name: write
**
** Description:
**	Send an input Unicode stream to the server as a series of
**	segments.  Segments are sized to fit the amount of space
**	remaining in the buffer.  Additional segments will end
**	the current message and begin a new message with the
**	same message ID.  The last segment is left in the buffer
**	so the caller may append additional data.
**
**	An OutputStreamWriter provides the conversion from Unicode
**	to the server character set and a ByteSegOS (special local
**	class) converts between the OutputStream format used by
**	OuputStreamWriter and the segment format of the bufferred
**	output stream.
**
** Input:
**	in	    Input Unicode stream.
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
**	 6-Sep-02 (gordy)
**	    Character encoding now encapsulated in CharSet class.
*/

public void
write( Reader in )
    throws EdbcEx
{
    OutputStreamWriter	os;

    if ( segOS == null )  segOS = new ByteSegOS( this, out, trace );
    segOS.begin( out_msg_id );	// begin a new BLOB.

    try { os = char_set.getOSW( segOS ); }
    catch( Exception ex )
    {
        // Should not happen!
        throw new EdbcEx( title + ": character encoding failed" );
    }

    try
    {
	int length;

	while( (length = in.read( char_buff, 0, char_buff.length )) >= 0 )  
	    os.write( char_buff, 0, length );

	os.flush();
    }
    catch( Exception ex )
    {
	throw EdbcEx.get( E_IO0007_BLOB_IO, ex );
    }
    finally
    {
	try { in.close(); } catch( Exception ignore ) {}
	try { os.close(); } catch( Exception ignore ) {}
	segOS.end();	// append the end-of-segments indicator.
    }

    return;
} // write


/*
** Name: write
**
** Description:
**	Send an input byte stream to the server as a series of
**	segments.  Segments are sized to fit the amount of space
**	remaining in the buffer.  Additional segments will end
**	the current message and begin a new message with the
**	same message ID.  The last segment is left in the buffer
**	so the caller may append additional data.
**
** Input:
**	in	    Input byte stream.
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
*/

public void
write( InputStream in )
    throws EdbcEx
{
    byte    msg_id = out_msg_id;
    short   seg_len;
    int	    len_pos;

    if ( trace.enabled( 3 ) )  trace.write( title + ": starting BLOB" );

    do
    {
	/*
	** A BLOB segment requires a minimum of three bytes:
	** two for the length indicator and one byte of data.
	*/
	if ( out.avail() < 3 )  
	{
	    done( false );	// Finish current message
	    begin( msg_id );	// Begin new message of same type.
	}

	/*
	** Write an initial length indicator of 0 (since
	** the segment has not been written yet) and save
	** the length indicator position for later updating.
	*/
	len_pos = out.position();
        out.write( (short)0 );

	/*
	** Now try to write a segment which fills the current
	** message buffer.  Update length indicator with the
	** actual segment length written.
	*/
	if ( (seg_len = out.write( in )) > 0 )  
	{
	    out.write( len_pos, seg_len );
	    if ( trace.enabled( 4 ) )
		trace.write( title + ": wrote " + seg_len + " of " + seg_len );
	}
    } while( seg_len > 0 );

    if ( trace.enabled( 3 ) )  trace.write( title + ": closing BLOB" );
    try { in.close(); } catch( Exception ignore ) {}
    return;
} // write


/*
** Name: done
**
** Description:
**	Finish the current message and optionally send
**	it to the server.
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
*/

public void
done( boolean flush )
    throws EdbcEx
{
    if ( out_hdr_pos > 0 )	// Update the message header.
    {
	short length = (short)( out.position() - 
				(out_hdr_pos + out_hdr_len) );

	if ( trace.enabled( 2 ) )  
	    trace.write( title + ": sending message " + 
			 IdMap.map( out_msg_id, MsgConst.msgMap ) + 
			 " length " + length + " EOD" + (flush ? " EOG" : "" ) );
	
	out.write( out_hdr_pos + 4, length );
	out.write( out_hdr_pos + 7, (byte)(JDBC_MSG_EOD | 
				    (flush ? JDBC_MSG_EOG : 0)) );
    }

    if ( flush )
	try { out.flush(); }
	catch( EdbcEx ex )
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
*/

private void
split()
    throws EdbcEx
{
    if ( out_hdr_pos > 0 )	
    {
	short length = (short)(out.position() - (out_hdr_pos + out_hdr_len));

	if ( trace.enabled( 3 ) )  
	    trace.write( title + ": splitting message " + 
			 IdMap.map( out_msg_id, MsgConst.msgMap ) + 
			 " length " + length );

	// Update the message header.
	try
	{
	    out.write( out_hdr_pos + 4, length );
	    out.write( out_hdr_pos + 7, (byte)0 );
	    out.flush();

	    out.begin( JDBC_TL_DT, 8 );		/* Begin continued message */
	    out_hdr_pos = out.position();
	    out.write( JDBC_MSG_ID );		/* Eyecatcher */
	    out.write( (short)0 );		/* Data length */
	    out.write( out_msg_id );		/* Message ID */
	    out.write( JDBC_MSG_EOD );		/* Flags */
	    out_hdr_len = out.position() - out_hdr_pos;
	}
	catch( EdbcEx ex )
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
**	An OutputStreamWriter object is required for conversion of 
**	Unicode BLOBs into server character encoded byte streams.  
**	OutputStreamWriter requires an OutputStream to process the
**	resulting byte stream.  This class extends OutputStream to
**	provide segmentation of the converted BLOB, bufferring the
**	stream as needed.
**
**	This class is designed for repeated usage.  The begin() and
**	end() methods provide the means for a single class instance
**	to process multiple serial BLOBs on the same connection.
**
**	This class is not static because it needs access to DbConnOut
**	begin(), done() and disconnect() methods.
**
**  Public Methods
**
**	begin		Initialize processing of a BLOB.
**	write		Write bytes in BLOB stream.
**	end		Complete processing of a BLOB.
**
**  Private Data
**
**	title		Class title for tracing.
**	trace		Tracing.
**	dbc_out		Db Connection output.
**	out		Bufferred output stream.
**	msg_id		Message ID of message containing the BLOB.
**	li_pos		Position of segment length indicator in output buffer.
**	seg_len		Length of the current segment.
**	ba		Temporary storage for writing single byte.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
**	17-Nov-99 (gordy)
**	    Made class nested (inner).
**	10-May-01 (gordy)
**	    Class renamed to match changes in DbConnIn.  Made class 
**	    static and added dbc_out for access to DB connection.
*/

private static class
ByteSegOS
    extends OutputStream
{

    private String	title;			// Class title for tracing.
    private Trace	trace;			// Tracing.

    private DbConnOut	dbc_out;		// Db Connection output.
    private OutBuff	out = null;		// Buffered output stream.
    private byte	msg_id = -1;		// Current message ID.

    private int		li_pos = -1;		// Position of length indicator.
    private short	seg_len = 0;		// Current segment length.
    private byte	ba[] = new byte[ 1 ];	// Temp storage.


/*
** Name: ByteSegOS
**
** Description:
**	Class constructor.
**
** Input:
**	dbc_out	Db Connection output.
**	out	Buffered output stream.
**	trace	Tracing.
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
**	    Added dbc_out parameter when class made static.
*/

public
ByteSegOS( DbConnOut dbc_out, OutBuff out, Trace trace )
{
    title = "ByteSegOS[" + dbc_out.connID() + "]";
    this.trace = trace;
    this.dbc_out = dbc_out;
    this.out = out;
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
*/

public void
begin( byte msg_id )
{
    if ( trace.enabled( 3 ) )  trace.write( title + ": start of BLOB" );
    this.msg_id = msg_id;
    li_pos = -1;
    seg_len = 0;
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
**	Write a byte sub-array to the segment stream.  The remainder
**	of the current message buffer is used to build a segment.
**	When the buffer is filled, the current message is completed
**	and a new message of the same type is started.  A non-zero
**	length segment is always left in the current output buffer.
**
** Input:
**	b	Byte array.
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
*/

public void
write( byte b[], int offset, int length )
    throws IOException
{
    try
    {
	if ( trace.enabled( 3 ) )
	    trace.write( title + ": writing " + length + " bytes" );

	while( length > 0 )
	{
	    int len;

	    if ( out.avail() < 3 )  
	    {
		li_pos = -1;
		dbc_out.done( false );	    // Finish current message
		dbc_out.begin( msg_id );    // Begin new message of same type.
	    }

	    if ( li_pos < 0 )
	    {
		li_pos = out.position();
		seg_len = 0;
		out.write( (short)0 );
	    }

	    len = Math.min( out.avail(), length );
	    out.write( b, offset, len );
	    offset += len;
	    length -= len;
	    seg_len += len;
	    out.write( li_pos, seg_len );

	    if ( trace.enabled( 4 ) )
		trace.write( title + ": wrote " + len + " of " + seg_len );
	}
    }
    catch( EdbcEx ex )
    { 
	if ( trace.enabled( 1 ) )
	{
	    trace.write( title + ": exception writing data" );
	    ex.trace( trace );
	}

	dbc_out.disconnect();
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
    catch( EdbcEx ex )
    { 
	if ( trace.enabled( 1 ) )
	{
	    trace.write( title + ": exception writing end-of-segments" );
	    ex.trace( trace );
	}

	dbc_out.disconnect();
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
*/

public void
end()
    throws EdbcEx
{
    if ( msg_id >= 0 )
    {
	if ( trace.enabled( 3 ) )  trace.write( title + ": closing BLOB" );

	if ( out.avail() < 2 )
	{
	    dbc_out.done( false );
	    dbc_out.begin( msg_id );
	}

	msg_id = -1;
	li_pos = -1;
	seg_len = 0;
	out.write( (short)0 );
    }

    return;
} // end

} // class ByteSegOS

} // class DbConnOut

