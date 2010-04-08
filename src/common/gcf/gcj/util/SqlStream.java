/*
** Copyright (c) 2007 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.util;

/*
** Name: SqlStream.java
**
** Description:
**	Defines base class which represents an SQL stream value.
**
**	Provides interface definitions for stream event listeners
**	and sources to support stream closure notification.
**
**  Classes:
**
**	SqlStream	An SQL stream value.
**	Rdr2IS		Convert Reader to InputStream.
**	Wtr2OS		Convert Writer to OutputStream.
**
**  Interfaces:
**
**	StreamListener	Listener for SqlStream events.
**	StreamSource	Source of SqlStream events.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Added support for parameter types/values in addition to 
**	    existing support for columns.
**	15-Nov-06 (gordy)
**	    Enhance conversion support.
**	 2-Feb-07 (gordy)
**	    Convert Writer streams to OutputStream.
**      17-Nov-08 (rajus01) SIR 121238
**	    Replaced SqlEx references with SQLException or SqlExFactory 
**	    depending upon the usage of it. SqlEx becomes obsolete to support 
**	    JDBC 4.0 SQLException hierarchy.
*/

import	java.io.InputStream;
import	java.io.OutputStream;
import	java.io.Reader;
import	java.io.Writer;
import	java.io.IOException;
import	java.nio.ByteBuffer;
import	java.nio.CharBuffer;
import	java.nio.charset.Charset;
import	java.nio.charset.CharsetEncoder;
import	java.nio.charset.CharsetDecoder;
import	java.nio.charset.CodingErrorAction;
import	java.nio.charset.CoderResult;
import	java.sql.SQLException;


/*
** Name: SqlStream
**
** Description:
**	Abstract Base class which represents an SQL stream value.  
**	SQL stream values are potentially large and variable length.  
**	Both binary and character based streams are supported via 
**	the InputStream and Reader Java IO objects (respectively).
**	Streams may only be read once, so only a single access is 
**	permitted to a stream value.  
**
**	Sub-classes must implement the cnvtIS2Rdr() method to convert
**	InputStream streams to Reader streams.  This method is used
**	by the methods getAscii(), getUnicode(), and getCharacter().
**	If a sub-class uses Reader streams directly or does not call
**	the indicated methods, cnvtIS2Rdr() may be implemented as a
**	stub.
**
**	A stream closure notification system is also supported via 
**	StreamListener and StreamSource interfaces.  A stream must
**	implement the StreamSource interface to participate in 
**	stream notification.
**
**  Abstract Methods:
**
**	cnvtIS2Rdr		Convert InputStream to Reader.
**
**  Public Methods:
**
**	copyIs2Os		Copy InputStream to OutputStream.
**	copyRdr2Wtr		Copy Reader to Writer.
**	getAsciiIS		Convert Reader to ASCII stream.
**	getAsciiOS		Convert Writer to ASCII stream.
**	getUnicodeIS		Convert Reader to Unicode stream.
**	setNull			Set a NULL data value.
**	toString		String representation of data value.
**	closeStream		Close the current stream.
**
**  Protected Methods:
**
**	setStream		Assign a new non-NULL data value.
**	getStream		Return current stream.
**	getBinary		Convert to binary InputStream.
**	getAscii		Convert to ASCII InputStream.
**	getUnicode		Convert to Unicode (UTF8) InputStream.
**	getCharacter		Convert to character Reader.
**
**  Private Data:
**
**	stream			Stream value.
**	active			Has stream been accessed?
**	listener		SqlStream event listener.
**	bbuff			Copy byte buffer.
**	cbuff			Copy character buffer.
**
**  Private Methods:
**
**	checkAccess		Check if access valid, mark accessed.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Removed requirement for StreamSource to support parameters.
**	    Added input-to-output copy methods and associated buffers.
**	15-Nov-06 (gordy)
**	    Added static utility methods getAsciiStream() and
**	    getUnicodeStream() to provide access to Rdr2IS class.
**	    Made copyIs2Os(), and copyRdr2Wtr() public & static
**	    for external access.  Made cbuff and bbuff static
**	    and added initializer.
**	 2-Feb-07 (gordy)
**	    Added static utility method setAsciiOS().  Renamed the
**	    companion methods to be more consistent.
*/

public abstract class
SqlStream
    extends SqlData
    implements GcfErr
{

    private Object		stream = null;		// InputStream/Reader
    private boolean		active = false;		// Has been accessed?
    private StreamListener	listener = null;	// Event listener.
    
    private static final byte	bbuff[] = new byte[ 8192 ];  // Copy buffers.
    private static final char	cbuff[] = new char[ 4096 ];
    

/*
** Abstract Methods
*/
protected abstract Reader cnvtIS2Rdr( InputStream stream ) throws SQLException;


/*
** Name: SqlStream
**
** Description:
**	Class constructor.  Data value is initially NULL.
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
**	12-Sep-03 (gordy)
**	    Created.
*/

protected
SqlStream()
{
    super( true );
} // SqlStream


/*
** Name: SqlStream
**
** Description:
**	Class constructor.  Data value is initially NULL.
**	Defines a SqlStream event listener for stream 
**	closure event notification.
**
** Input:
**	listener	Stream event listener.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
*/

protected
SqlStream( StreamListener listener )
{
    this();
    this.listener = listener;
} // SqlStream


/*
** Name: setNull
**
** Description:
**	Set the NULL state of the data value to NULL.
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
**	12-Sep-03 (gordy)
**	    Created.
*/

public synchronized void
setNull()
{
    super.setNull();
    stream = null;
    active = false;
    return;
} // setNull


/*
** Name: toString
**
** Description:
**	Returns a string representing the current object.
**
**	The SqlData super-class calls getString() to implement
**	this method, which is not necessarily a good thing for
**	all streams.  Override to simply call the toString()
**	method of the current stream (watching out for NULL
**	values).
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	String representation of this SQL data value.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
*/

public String
toString()
{
    return( "SqlStream: " + ((stream == null) ? "NULL" : stream.toString()) );
} // toString


/*
** Name: closeStream
**
** Description:
**	Close associated stream.
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
**	12-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    StreamSource no longer required, so check for stream classes.
*/

public synchronized void
closeStream()
{
    active = true;	// Don't allow access after close
    if ( stream == null )  return;
    
    try 
    { 
	if ( stream instanceof InputStream )
	    ((InputStream)stream).close();
	else  if ( stream instanceof Reader )
	    ((Reader)stream).close(); 
    }
    catch( IOException ignore ) {}
    
    return;
} // close


/*
** Name: setStream
**
** Description:
**	Assign a new stream data value.  The data value will be 
**	NULL if the input value is null, otherwise non-NULL.  The 
**	data object must be an instance of InputStream or Reader 
**	and may optionally implement the StreamSource interface.
**
** Input:
**	stream	InputStream or Reader.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted from set(), StreamSource no longer required.
*/

protected synchronized void
setStream( Object stream )
    throws SQLException
{
    if ( stream == null )
	setNull();
    else
    {
	if ( 
	     ! (stream instanceof InputStream)  &&
	     ! (stream instanceof Reader) 
	   )
	    throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
	
	setNotNull();
	active = false;
	this.stream = stream;
	
	if ( stream instanceof StreamSource )  
	    ((StreamSource)stream).addStreamListener( listener, this );
    }
    
    return;
} // set


/*
** Name: getStream
**
** Description:
**	Return the current stream data value.
**
**	Note: the value returned when the data value is 
**	NULL is not well defined.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Object	Current stream value.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

protected Object
getStream()
    throws SQLException
{
    checkAccess();
    return( stream );
} // getStream


/*
** Name: getBinary
**
** Description:
**	Returns a binary InputStream representation of the
**	current stream.  Enforces single access to the stream.
**	Base stream must be of type InputStream.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	InputStream	Binary stream.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Check for NULL stream.
*/

protected InputStream
getBinary()
    throws SQLException
{
    checkAccess();
    
    if ( stream == null )
	return( null );
    else  if ( stream instanceof InputStream )
	return( (InputStream)stream );
    else
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
} // getBinary


/*
** Name: getAscii
**
** Description:
**	Returns an ASCII InputStream representation of the
**	current stream.  Enforces single access to the stream.
**	Byte streams are converted to a hex character stream
**	prior to conversion to ASCII.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	InputStream	ASCII stream.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Check for NULL stream.
**	15-Nov-06 (gordy)
**	    Use new utility method getAsciiStream().
*/

protected InputStream
getAscii()
    throws SQLException
{
    checkAccess();
    
    if ( stream == null )
	return( null );
    else  if ( stream instanceof InputStream )
	return( getAsciiIS( cnvtIS2Rdr( (InputStream)stream ) ) );
    else  if ( stream instanceof Reader )
	return( getAsciiIS( (Reader)stream ) );
    else
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
} // getAscii


/*
** Name: getUnicode
**
** Description:
**	Returns an UTF8 InputStream representation of the
**	current stream.  Enforces single access to the stream.
**	Byte streams are converted to a hex character stream
**	prior to conversion to UTF8.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	InputStream	UTF8 stream.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Check for NULL stream.
**	15-Nov-06 (gordy)
**	    Use new utility method getUnicodeStream().
*/

protected InputStream
getUnicode()
    throws SQLException
{
    checkAccess();
    
    if ( stream == null )
	return( null );
    else  if ( stream instanceof InputStream )
	return( getUnicodeIS( cnvtIS2Rdr( (InputStream)stream ) ) );
    else  if ( stream instanceof Reader )
	return( getUnicodeIS( (Reader)stream ) );
    else
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
} // getUnicode


/*
** Name: getCharacer
**
** Description:
**	Returns a character Reader representation of the
**	current stream.  Enforces single access to the stream.
**	Byte streams are converted to a hex character stream.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Reader		Character stream.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Check for NULL stream.
*/

protected Reader
getCharacter()
    throws SQLException
{
    checkAccess();
    
    if ( stream == null )
	return( null );
    else  if ( stream instanceof InputStream )
	return( cnvtIS2Rdr( (InputStream)stream ) );
    else  if ( stream instanceof Reader )
	return( (Reader)stream );
    else
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
} // getCharacter


/*
** Name: checkAccess
**
** Description:
**	Check state of stream.  Mark as accessed if permitted.
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
**	12-Sep-03 (gordy)
**	    Created.
*/

private synchronized void
checkAccess()
    throws SQLException
{
    if ( active )  throw SqlExFactory.get( ERR_GC401C_BLOB_DONE );
    active = true;
    return;
} // checkAccess


/*
** Name: StreamListener
**
** Description:
**	Interface defining a listener for SqlStream events.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
*/

public interface
StreamListener
{

/*
** Name: streamClosed
**
** Description:
**	Indicates that a SQL Stream object has been closed.
**
** Input:
**	stream		SQL Stream which has closed.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
*/

void streamClosed( SqlStream stream );


} // interface StreamListener


/*
** Name: StreamSource
**
** Description:
**	Interface defining a source of SqlStream events.
**
**  Interface Methods:
**
**	addStreamListener	Add a SqlStream event listener.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Removed close() method.  Appropriatte method of actual
**	    stream class should be used instead.
*/

public interface
StreamSource
{

/*
** Name: addStreamListener
**
** Description:
**	Add a StreamListener to a StreamSource.  Only a single
**	listener needs to be supported and a NULL listener 
**	removes all registered listeners. The SqlStream to be
**	returned to the listener is also provided.
**
** Input:
**	listener	SqlStream listener (may be NULL).
**	stream		SqlStream associated with source/listener.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
*/

void addStreamListener( StreamListener listener, SqlStream stream );


} // interface StreamSource


/*
** Name: copyIs2Os
**
** Description:
**	Writes the contents of an InputStream to an OutputStream.
**	The InputStream is closed.  The OutputStream is flushed
**	but not closed.
**
** Input:
**	is	InputStream.
**	os	OutputStream.
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
**	15-Nov-06 (gordy)
**	    Made public & static for external usage.
*/

public static void
copyIs2Os( InputStream is, OutputStream os )
    throws SQLException
{
    try
    {
	synchronized( bbuff )
	{
	    for( int len = is.read(bbuff); len >= 0; len = is.read(bbuff) )
		os.write( bbuff, 0, len );
	}

	is.close();
	os.flush();
    }
    catch( Exception ex )
    { 
	try { is.close(); } catch( Exception ignore ) {} 
	try { os.flush(); } catch( Exception ignore ) {} 
	throw SqlExFactory.get( ERR_GC4007_BLOB_IO, ex ); 
    }
    
    return;
} // copyIs2Os


/*
** Name: coyRdr2Wtr
**
** Description:
**	Writes the contents of a Reader stream to a Writer stream.
**	The Reader stream is closed.  The Writer stream is flushed
**	but not closed.
**
** Input:
**	rdr	Reader stream.
**	wtr	Writer stream.
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
**	15-Nov-06 (gordy)
**	    Made public & static for external usage.
*/

public static void
copyRdr2Wtr( Reader rdr, Writer wtr )
    throws SQLException
{
    try
    {
	synchronized( cbuff )
	{
	    for( int len = rdr.read(cbuff); len >= 0; len = rdr.read(cbuff) )
		wtr.write( cbuff, 0, len );
	}

	rdr.close();
	wtr.flush();
    }
    catch( Exception ex )
    { 
	try { rdr.close(); } catch( Exception ignore ) {} 
	try { wtr.flush(); } catch( Exception ignore ) {} 
	throw SqlExFactory.get( ERR_GC4007_BLOB_IO, ex ); 
    }
    
    return;
} // copyRdr2Wtr


/*
** Name: getAsciiIS
**
** Description:
**	Converts a character Reader into an ASCII stream.
**
** Input:
**	reader		Character Reader
**
** Output:
**	None.
**
** Returns:
**	InputStream	ASCII stream.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public static InputStream
getAsciiIS( Reader reader )
{
    return( new Rdr2IS( reader, "US-ASCII" ) );
} // getAsciiIS


/*
** Name: getAsciiOS
**
** Description:
**	Converts a character Writer into an ASCII stream.
**
** Input:
**	Writer		Character Writer
**
** Output:
**	None.
**
** Returns:
**	OutputStream	ASCII stream.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public static OutputStream
getAsciiOS( Writer writer )
{
    return( new Wtr2OS( writer, "US-ASCII" ) );
} // getAsciiOS


/*
** Name: getUnicodeIS
**
** Description:
**	Converts a character Reader into a Unicode stream.
**
** Input:
**	reader		Character Reader
**
** Output:
**	None.
**
** Returns:
**	InputStream	Unicode stream.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public static InputStream
getUnicodeIS( Reader reader )
{
    return( new Rdr2IS( reader, "UTF-8" ) );
} // getUnicodeIS


/*
** Name: Rdr2IS
**
** Description:
**	Class which converts Reader stream to InputStream using
**	a designated character-set.
**
**  Public Methods:
**
**	read
**	skip
**	close
**
**  Constants:
**
**	EOI_RDR			End-of-input: reader stage.
**	EOI_ENC			End-of-input: encoder stage.
**	EOI_OUT			End-of-input: output stage.
**	EOI			End-of-input: all stages.
**	CLOSED			Stream closed.
**
**  Private Data:
**
**	eoi_flags		End-of-input stage flags.
**	rdr			Input Reader stream.
**	encoder			Character-set encoder.
**	inBuff			Input character buffer.
**	outBuff			Output byte buffer.
**	ba			Temporary byte buffer.
**
**  Private Methods:
**
**	fillOutput		Fill output buffer.
**	fillInput		Fill input buffer.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	 2-Feb-07 (gordy)
**	    Removed read() method which is efficiently implemented 
**	    by base class.  Added CLOSED flag.
*/

private static class
Rdr2IS
    extends InputStream
{
    
    private static final int	EOI_RDR	= 0x01;		// Reader finished.
    private static final int	EOI_ENC	= 0x02;		// Encoder finished.
    private static final int	EOI_OUT	= 0x04;		// Output finished.
    private static final int	CLOSED	= 0x08;
    private static final int	EOI	= EOI_RDR | EOI_ENC | EOI_OUT;
    
    private int			eoi_flags = 0;
    private Reader		rdr = null;
    private CharsetEncoder	encoder = null;
    private CharBuffer		inBuff = CharBuffer.allocate( 8192 );
    private ByteBuffer		outBuff = ByteBuffer.allocate( 8192 );
    private byte		ba[] = new byte[1];

    
/*
** Name: Rdr2IS
**
** Description:
**	Class constructor.
**
** Input:
**	rdr		Reader.
**	charSet		Character-set name.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
*/

public
Rdr2IS( Reader rdr, String charSet )
{
    this.rdr = rdr;
    encoder = Charset.forName( charSet ).newEncoder();
    encoder.onMalformedInput( CodingErrorAction.REPLACE );
    encoder.onUnmappableCharacter( CodingErrorAction.REPLACE );
    
    /*
    ** Initialize input/output buffers as empty so that 
    ** the first read() request will proceed to fill them.
    ** The initial buffer state is clear() for reading,
    ** so simply flip() ready for writing.
    */
    inBuff.flip();
    outBuff.flip();
} // Rdr2IS


/*
** Name: close
**
** Description:
**	Close the input stream.
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
**	12-Sep-03 (gordy)
**	    Created.
**	 2-Feb-07 (gordy)
**	    Added CLOSED flag.
*/

public void
close()
    throws IOException
{
    if ( (eoi_flags & CLOSED) == 0 )
    {
	eoi_flags = EOI | CLOSED;	// Force end-of-input state.
	rdr.close();
    }
    return;
} // close


/*
** Name: read
**
** Description:
**	Read next byte.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int    Next byte (0-255) or -1 at end-of-input.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
*/

public int
read()
    throws IOException
{
    return( (read( ba, 0, 1 ) == -1) ? -1 : (int)ba[0] & 0xff );
} // read


/*
** Name: read
**
** Description:
**	Read bytes into a sub-array.
**
** Input:
**	offset	Starting position.
**	length	Number of bytes.
**
** Output:
**	bytes	Byte array.
**
** Returns:
**	int	Number of bytes read or -1 if end-of-input.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	 2-Feb-07 (gordy)
**	    Validate parameters.
*/

public int
read( byte bytes[], int offset, int length )
    throws IOException
{
    /*
    ** Validate parameters.
    */
    if ( (eoi_flags & CLOSED) != 0 )  throw new IOException( "Stream closed" );
    if ( bytes == null )  throw new NullPointerException();
    if ( offset < 0  ||  length < 0  ||  bytes.length - offset < length )
	throw new IndexOutOfBoundsException();

    int limit = offset + length;	// Save boundaries
    int start = offset;
    
    if ( (eoi_flags & EOI_OUT) == 0 )	// Reached end-of-input?
	while( offset < limit )		// More data to transer
	{
	    /*
	    ** Fill output buffer when empty.
	    */
	    if ( ! outBuff.hasRemaining()  &&  ! fillOutput() )
	    {
	        eoi_flags |= EOI_OUT;	// End-of-input!
		break;
	    }

	    /*
	    ** Return available data
	    */
	    length = Math.min( limit - offset, outBuff.remaining() );
	    outBuff.get( bytes, offset, length );
	    offset += length;
	}
    
    /*
    ** If some data was transferred, return the length
    ** irregardless if end-of-input has been reached.
    ** End-of-input only returned when no data has been
    ** transferred.  Make sure some data was requested
    ** otherwise a false end-of-input would be returned.
    */
    return( (length > 0  &&  offset == start) ? -1 : offset - start );
} // read


/*
** Name: skip
**
** Description:
**	Skip bytes in input stream.
**
** Input:
**	count	Number of bytes to skip.
**
** Output:
**	None.
**
** Returns:
**	long	Number of bytes skipped.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	 2-Feb-07 (gordy)
**	    Check for stream closed.
*/

public long
skip( long count )
    throws IOException
{
    if ( (eoi_flags & CLOSED) != 0 )  throw new IOException( "Stream closed" );

    long start = count;			// Save original
    
    if ( (eoi_flags & EOI_OUT) == 0 )	// Reached end-of-input?
	while( count > 0 )		// More bytes to skip
	{
	    /*
	    ** Fill output buffer when empty.
	    */
	    if ( ! outBuff.hasRemaining()  &&  ! fillOutput() )
	    {
	        eoi_flags |= EOI_OUT;	// End-of-input!
	        break;
	    }
		
	    /*
	    ** Data is skipped simply by resetting the 
	    ** output buffer position.
	    */
	    long length = Math.min( count, (long)outBuff.remaining() );
	    outBuff.position( outBuff.position() + (int)length );
	    count -= length;
	}
    
    return( start - count );		// Amount skipped
} // skip


/*
** Name: fillOutput
**
** Description:
**	Encodes characters from input buffer into the byte
**	output buffer.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if data available, False if EOI.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	 2-Feb-07 (gordy)
**	    Make sure encoder completes before setting EOI.
*/

private boolean
fillOutput()
    throws IOException
{
    outBuff.clear();			// Prepare for reading.
    
    if ( (eoi_flags & EOI_ENC) == 0 )	// Reached end-of-input?
	do
	{
	    /*
	    ** Fill input buffer when empty.
	    */
	    if ( ! inBuff.hasRemaining()  &&  ! fillInput() )
	    {
		/*
		** We only want to flush the encoder once,
		** and then only with an empty output buffer
		** to ensure that there will be room for any
		** flushed data.  If anything has been placed
		** in the output buffer, the end-of-input
		** processing will be done on the next call.
		** Likewise, only flag EOI when encoder is done.
		*/
		if ( 
		     outBuff.position() == 0  &&
		     encoder.encode( inBuff, outBuff, 
				     true ) == CoderResult.UNDERFLOW  &&
		     encoder.flush( outBuff ) == CoderResult.UNDERFLOW
		   )
		    eoi_flags |= EOI_ENC;
		break;
	    }

	    /*
	    ** Encode input characters into output bytes.
	    ** Loop to fill input buffer when empty.  Only
	    ** UNDERFLOW or OVERFLOW should be returned 
	    ** since the encoder has been set to REPLACE 
	    ** or IGNORE other coding errors.
	    */
	} while( encoder.encode( inBuff, 
				 outBuff, false ) == CoderResult.UNDERFLOW );
    
    /*
    ** Only end-of-input can keep something from being
    ** placed in the output buffer.  We may have hit
    ** end-of-input and still placed something in the
    ** output buffer, in which case end-of-input will
    ** be signaled on the next call.
    */
    outBuff.flip();			// Prepare for writing.
    return( outBuff.hasRemaining() );
} // fillOutput


/*
** Name: fillInput
**
** Description:
**	Reads characters into input buffer from Reader stream.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean		True if data available, False if EOI.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
*/

private boolean
fillInput()
    throws IOException
{
    boolean ready = false;
    inBuff.clear();			// Prepare for reading.
    
    if ( (eoi_flags & EOI_RDR) == 0 )	// Reached end-of-input?
    {
	/*
	** The Reader class does not directly support
	** transfer of data with our input buffer, so
	** need to read directly into the underlying
	** character array (buffer allocation method
	** assures the presence of the array).
	*/
	int length = rdr.read( inBuff.array() );

	/*
	** Check for end-of-input.
	*/
	if ( length < 0 )
	    eoi_flags |= EOI_RDR;
	else
	{
	    /*
	    ** Update buffer with amount transferred.
	    */
	    inBuff.position( inBuff.position() + length );
	    ready = true;
	}
    }
    
    inBuff.flip();			// Prepare for writing.
    return( ready );
} // fillInput


} // class Rdr2IS


/*
** Name: Wtr2OS
**
** Description:
**	Class which converts Writer stream to OutputStream using
**	a designated character-set.
**
**  Public Methods:
**
**	close
**	flush
**	write
**
**  Private Data:
**
**	closed			Is stream closed?
**	wtr			Output Writer stream.
**	decoder			Character-set decoder.
**	inBuff			Input character buffer.
**	outBuff			Output byte buffer.
**	ba			Temporary byte buffer.
**
**  Private Methods:
**
**	flushInput		Flush input buffer.
**	flushOutput		Flush output buffer.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

private static class
Wtr2OS
    extends OutputStream
{
    
    private boolean		closed = false;
    private Writer		wtr = null;
    private CharsetDecoder	decoder = null;
    private ByteBuffer		inBuff = ByteBuffer.allocate( 8192 );
    private CharBuffer		outBuff = CharBuffer.allocate( 8192 );
    private byte		ba[] = new byte[1];

    
/*
** Name: Wtr2OS
**
** Description:
**	Class constructor.
**
** Input:
**	wtr		Writer.
**	charSet		Character-set name.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public
Wtr2OS( Writer wtr, String charSet )
{
    this.wtr = wtr;
    decoder = Charset.forName( charSet ).newDecoder();
    decoder.onMalformedInput( CodingErrorAction.REPLACE );
    decoder.onUnmappableCharacter( CodingErrorAction.REPLACE );
    
    /*
    ** Clear input/output buffers in preparation to be filled.
    */
    inBuff.clear();
    outBuff.clear();
} // Wtr2OS


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
**	 2-Feb-07 (gordy)
**	    Created.
*/

public void
close()
    throws IOException
{
    if ( ! closed )
    {
	closed = true;

	/*
	** Forcefully flush entire processesing pipeline: 
	** input buffer, decoder, output buffer, and stream.
	*/
	flushInput( true );

	while( decoder.flush( outBuff ) == CoderResult.OVERFLOW )
	    flushOutput( false );

	flushOutput( true );
	wtr.close();
    }
    return;
} // close


/*
** Name: flush
**
** Description:
**	Flush buffered data to output stream.
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
**	 2-Feb-07 (gordy)
*/

public void
flush()
    throws IOException
{
    if ( closed )  throw new IOException( "Stream closed" );
    flushInput( false );
    flushOutput( false );
    return;
} // flush


/*
** Name: Write
**
** Description:
**	Write a single byte.
**
** Input:
**	data	Byte to be written.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public void
write( int data )
    throws IOException
{
    ba[0] = (byte)(data & 0xff);
    write( ba, 0, 1 );
    return;
} // write


/*
** Name: write
**
** Description:
**	Write bytes from a sub-array.
**
** Input:
**	offset	Starting position.
**	length	Number of bytes.
**
** Output:
**	bytes	Byte array.
**
** Returns:
**	void
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public void
write( byte bytes[], int offset, int length )
    throws IOException
{
    /*
    ** Validate parameters.
    */
    if ( closed )  throw new IOException( "Stream closed" );
    if ( bytes == null )  throw new NullPointerException();
    if ( offset < 0  ||  length < 0  ||  bytes.length - offset < length )
	throw new IndexOutOfBoundsException();

    while( length > 0 )
    {
	/*
	** Flush input buffer when full.
	*/
	if ( ! inBuff.hasRemaining() )  flushInput( false );

	/*
	** Fill input buffer.
	*/
	int len = Math.min( length, inBuff.remaining() );
	inBuff.put( bytes, offset, len );
	offset += len;
	length -= len;
    }

    return;
} // write


/*
** Name: flushInput
**
** Description:
**	Decodes bytes from input buffer into the character
**	output buffer.
**
** Input:
**	eoi	End-of-input.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

private void
flushInput( boolean eoi )
    throws IOException
{
    inBuff.flip();			// Prepare for reading.
    
    do
    {
	/*
	** Flush output buffer when full.
	*/
	if ( ! outBuff.hasRemaining() )  flushOutput( false );

	/*
	** Decode input bytes into output characters.
	** Loop to flush output buffer when full.  Only
	** UNDERFLOW or OVERFLOW should be returned
	** since the decoder has been set to REPLACE 
	** or IGNORE other coding errors.
	*/
    } while( decoder.decode( inBuff, outBuff, eoi ) == CoderResult.OVERFLOW );

    inBuff.clear();			// Prepare for writing.
    return;
} // flushInput


/*
** Name: flushOutput
**
** Description:
**	Write characters from output buffer to Writer stream.
**	Stream can optionally be flushed as well.
**
** Input:
**	force	Flush stream.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

private void
flushOutput( boolean force )
    throws IOException
{
    outBuff.flip();			// Prepare for writing.

    /*
    ** Write buffered data to stream and flush if requested.
    */
    if ( outBuff.hasRemaining() )
	wtr.write( outBuff.array(), outBuff.position(), outBuff.remaining() );
    if ( force )  wtr.flush();
 
    outBuff.clear();			// Prepare for filling.
    return;
} // flushOutput


} // class Wtr2OS


} // class SqlStream
