/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.io;

/*
** Name: IoBuff.java
**
** Description:
**	Defines the Ingres JDBC Driver class IoBuff which supports
**	buffered client-server I/O at the Ingres/Net TCP-IP driver
**	level.
**
** History:
**	 9-Jun-99 (gordy)
**	    Created.
**	13-Sep-99 (gordy)
**	    Implemented error code support.
**	29-Sep-99 (gordy)
**	    A recursive loop can occur if a flush fails, which
**	    calls close() which calls flush(), etc.  Made sure
**	    close() won't recurse by setting the closed flag
**	    prior to calling flush().
**	28-Jan-00 (gordy)
**	    Added TL segment tracing.
**	20-Apr-00 (gordy)
**	    Moved to package io.
**	28-Mar-01 (gordy)
**	    Separated tracing interface and implementation.
*/

import	java.io.InputStream;
import	java.io.OutputStream;
import	ca.edbc.util.EdbcEx;
import	ca.edbc.util.Trace;
import	ca.edbc.util.Tracing;


/*
** Name: IoBuff
**
** Description:
**	Implements buffered client-server I/O corresponding to the
**	Ingres/Net TCP-IP driver (Network Layer) protocol.  Block
**	buffer operations are supported, but no direct buffer access
**	is provided publically.  Instances of this class support
**	either input or output, not both.  It is expected that sub-
**	classes will be defined providing input or output access to 
**	the I/O buffer.
**
**	The Ingres/Net TCP-IP drivers internally support segmentation
**	of the data stream.  Data segments are exposed to sub-classes
**	to support concatenation without requiring buffer copies.
**
**	Access to the I/O buffer and related data is not multi-threaded
**	protected at this level.  Sub-classes must provide protection
**	or define access requirements to assure thread-safe access.
**	Public and protected methods which access the I/O stream are
**	synchronized.  Its not entirely obvious that this is required,
**	since the same conventions which protect the I/O buffer should
**	protect the I/O stream.  Synchronization is provided because of
**	the close() method which may fall outside the protection of the
**	I/O buffer.
**
**  Public Methods:
**
**	setBuffSize	Set the size of the I/O buffer.
**	clear		Clear the I/O buffer.
**	close		Close the associated I/O stream.
**
**  Protected Data:
**
**	title		Class title for tracing.
**	trace		Tracing for class.
**	buffer		The I/O buffer.
**	data_beg	Start of current data.
**	data_ptr	Current I/O position.
**	data_end	End of data (input) or buffer (output).
**
**  Protected Methods:
**
**	begin		Start a new segment in the I/O buffer.
**	flush		Send segments in the I/O buffer to server.
**	next		Read next segment in buffer/stream.
**
**  Private Data:
**
**	in		The input data stream.
**	out		The output data stream.
**	closed		Has data stream been closed?
**	buf_max		Size of I/O buffer.
**	buf_len		End of data (multi-segment).
**	seg_beg		Start of current segment.
**	seg_end		End of current segment.
**
**  Private Methods:
**
**	clearBuffer	Initialize I/O buffer as empty.
**	flushBuffer	Write buffer on output stream.
**	fillBuffer	Read buffer from input stream.
**	getSegSize	Read segment length in segment header.
**	setSegSize	Update the segment header with segment length.
**
** History:
**	 9-Jun-99 (gordy)
**	    Created.
**	20-Apr-00 (gordy)
**	    Removed super class EdbcObject.
*/

class
IoBuff
    implements IoConst, IoErr
{

    private InputStream	    in = null;		// Input data stream.
    private OutputStream    out = null;		// Output data stream.
    private boolean	    closed = false;	// Is data stream closed?
    private int		    buf_max;		// Buffer size.
    private int		    buf_len;		// End of data.
    private int		    seg_beg;		// Start of current segment.
    private int		    seg_end;		// End of current segment.

    protected byte	    buffer[];		// IO buffer.
    protected int	    data_beg;		// Start of data.
    protected int	    data_ptr;		// Current data position.
    protected int	    data_end;		// End of data/buffer.

    protected String	    title;		// Object title for tracing.
    protected Trace	    trace;		// Tracing.


/*
** Name: IoBuff
**
** Description:
**	Class constructor for input buffers.
**
** Input:
**	in	The associated input stream.
**	size	Size of I/O buffer.
**
** Output:
**	None.
**
** History:
**	 9-Jun-99 (gordy)
**	    Created.
**	28-Mar-01 (gordy)
**	    Separated tracing interface and implementation.
*/

protected
IoBuff( InputStream in, int size )
{
    title = "IoBuff[in]";
    trace = new Tracing( "IO" );
    this.in = in;
    setBuffSize( size );
} // IoBuff

/*
** Name: IoBuff
**
** Description:
**	Class constructor for output buffers.
**
** Input:
**	out	The associated output stream.
**	size	Size of I/O buffer.
**
** Output:
**	None.
**
** History:
**	 9-Jun-99 (gordy)
**	    Created.
**	28-Mar-01 (gordy)
**	    Separated tracing interface and implementation.
*/

protected
IoBuff( OutputStream out, int size )
{
    title = "IoBuff[out]";
    trace = new Tracing( "IO" );
    this.out = out;
    setBuffSize( size );
} // IoBuff

/*
** Name: clearBuffer
**
** Description:
**	Initialize the I/O buffer as empty.
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
**	 9-Jun-99 (gordy)
**	    Created.
*/

private void
clearBuffer()
{
    seg_beg = seg_end = buf_len = 0;
    data_beg = data_ptr = data_end = 0;
    return;
} // clearBuffer

/*
** Name: flushBuffer
**
** Description:
**	Write buffer to output stream and clear output buffer.
**	The output stream is not actually flushed, since this
**	routine may be called between a pair of buffers.
**
** Input:
**	force	Force socket to flush buffers?
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 9-Jun-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Moved socket flush (optional) to this routine so
**	    it can be requested from local methods without the
**	    overhead of calling flush().
*/

private void
flushBuffer( boolean force )
    throws EdbcEx
{
    if ( buf_len <= 0 )  return;

    if ( trace.enabled( 4 ) )  
    {
	trace.write( title + ": sending " + buf_len + " bytes." );
	trace.hexdump( buffer, 0, buf_len );
    }

    try { out.write( buffer, 0, buf_len ); }
    catch( Exception ex )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( title + ": write error: " + ex.getMessage() );
	clearBuffer();
	close();
	throw EdbcEx.get( E_IO0005_CONNECT_FAIL, ex );
    }

    clearBuffer();

    if ( force )
	try { out.flush(); }
	catch( Exception ex )
	{
	    if ( trace.enabled( 1 ) )
		trace.write( title + ": flush error: " + ex.getMessage() );
	    close();
	    throw EdbcEx.get( E_IO0005_CONNECT_FAIL, ex );
	}

    return;
} // flushBuffer

/*
** Name: fillBuffer
**
** Description:
**	Read buffer from input stream if requested amount of data
**	is not currently available ((seg_beg + size) > buf_len).
**
** Input:
**	size	Amount of data required.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 9-Jun-99 (gordy)
**	    Created.
*/

private void
fillBuffer( int size )
    throws EdbcEx
{
    /*
    ** Check to see if data is currently available.
    */
    if ( (seg_beg + size) <= buf_len )  return;

    /*
    ** The buffer size should have been negotiated
    ** at connection time and any segment should
    ** fit in the buffer.
    */
    if ( size > buf_max )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( title + ": buffer overflow (" + size + "," + buf_max + ")" );
	close();
	throw EdbcEx.get( E_IO0002_PROTOCOL_ERR );
    }

    /*
    ** Maximize space available for new data.
    */
    if ( seg_beg > 0 )
    {
	int offset;

	for( offset = 0; (seg_beg + offset) < buf_len; offset++ )
	    buffer[ offset ] = buffer[ seg_beg + offset ];

	buf_len -= seg_beg;
	seg_end -= seg_beg;
	seg_beg = 0;
    }

    while( buf_len < size )
    {
	int length;

	if ( trace.enabled( 5 ) )
	    trace.write( title + ": reading " + (buf_max - buf_len) + " bytes (" + 
			 size + " requested, " + buf_len + " buffered)" );

	try { length = in.read( buffer, buf_len, buf_max - buf_len ); }
	catch( Exception ex )
	{
	    if ( trace.enabled( 1 ) )  
		trace.write( title + ": read error: " + ex.getMessage() );
	    close();
	    throw EdbcEx.get( E_IO0005_CONNECT_FAIL, ex );
	}

	if ( length < 0 )
	{
	    if ( trace.enabled( 1 ) )  
		trace.write( title + ": connection closed by server" );
	    close();
	    throw EdbcEx.get( E_IO0005_CONNECT_FAIL );
	}

	if ( trace.enabled( 4 ) )  
	{
	    trace.write( title + ": received " + length + " bytes." );
	    trace.hexdump( buffer, buf_len, length );
	}

	buf_len += length;
    }

    return;
} // fillBuffer

/*
** Name: getSegSize
**
** Description:
**	Reads the Ingres/Net TCP-IP header and returns the length
**	of the current segment.  The current position (buf_beg) is
**	moved past the TCP-IP header.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Segment length.
**
** History:
**	 9-Jun-99 (gordy)
**	    Created.
*/

private int
getSegSize()
{
    return( (buffer[ seg_beg ] & 0x00ff) | 
	    ((buffer[ seg_beg + 1 ] << 8) & 0xff00) );
} // getSegSize

/*
** Name: setSegSize
**
** Description:
**	Updates the Ingres/Net TCP-IP header with the length
**	of the current segment.
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
**	 9-Jun-99 (gordy)
**	    Created.
**	28-Jan-00 (gordy)
**	    Added TL segment tracing.
*/

private void
setSegSize()
{
    int size;

    /*
    ** For output, data_ptr points at the end of
    ** the data written by our subclass.
    */
    if ( data_ptr <= (seg_beg + 2) )  data_ptr = seg_beg;  // just to be safe
    seg_end = buf_len = data_ptr;
    size = seg_end - seg_beg;

    if ( size > 0 )
    {
	if ( trace.enabled( 5 ) )  
	    trace.write( title + ": write segment " + size );

	/*
	** The Ingres NL standard requires little endian
	** for the length indicator.
	*/
	buffer[ seg_beg ] = (byte)(size & 0xff);
	buffer[ seg_beg + 1 ] = (byte)((size >> 8) & 0xff);
    }

    return;
} // setSegSize

/*
** Name: begin
**
** Description:
**	Begin a new Ingres NL data segment in the output buffer.
**	reserves space for the Ingres TCP-IP header.  Flushes
**	current buffer if insufficient space for next segment.
**
** Input:
**	size	    Minimum amount of space required for next segment.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 9-Jun-99 (gordy)
**	    Created.
*/

protected synchronized void
begin( int size )
    throws EdbcEx
{
    if ( closed  ||  out == null )
	throw EdbcEx.get( E_IO0004_CONNECTION_CLOSED );

    setSegSize();	// Update the current segment length (if any).
    seg_beg = buf_len;	// New segment starts at end of current data.

    /*
    ** Flush current segments if insufficient space
    ** for next segment.  Note that flushBuffer()
    ** handles the case of no current segments.
    */
    if ( (buf_len + size + 2) > buf_max )  flushBuffer( false );

    /*
    ** Reserve space for the Ingres TCP-IP header.
    */
    data_beg = data_ptr = seg_beg + 2;
    data_end = buf_max;

    return;
} // begin

/*
** Name: flush
**
** Description:
**	Send output buffer to server.
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
**	 9-Jun-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Moved socket flush to flushBuffer().
*/

protected synchronized void
flush()
    throws EdbcEx
{
    if ( closed  ||  out == null )
	throw EdbcEx.get( E_IO0004_CONNECTION_CLOSED );

    if ( trace.enabled( 3 ) )
	trace.write(title + ": sending message length " + (data_ptr - data_beg));
    setSegSize();	// Update current segment length.
    flushBuffer( true );
    return;
} // flush

/*
** Name: next
**
** Description:
**	Read next segment from input stream.
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
**	 9-Jun-99 (gordy)
**	    Created.
**	28-Jan-00 (gordy)
**	    Added TL segment tracing.
*/

protected synchronized void
next()
    throws EdbcEx
{
    int size;

    if ( closed  ||  in == null )
	throw EdbcEx.get( E_IO0004_CONNECTION_CLOSED );

    if ( seg_beg < seg_end )  seg_beg = seg_end;
    if ( seg_beg >= buf_len )  clearBuffer();

    fillBuffer( 2 );		// Make sure length is available
    size = getSegSize();
    if ( trace.enabled( 5 ) )  trace.write( title + ": read segment " + size );

    fillBuffer( size );		// Make sure entire segment is available
    seg_end = seg_beg + size;
    data_beg = data_ptr = seg_beg + 2;
    data_end = seg_end;

    return;
} // next

/*
** Name: setBuffSize
**
** Description:
**	Allocate and initialize the I/O buffer.  Any previous
**	data is lost.
**
** Input:
**	size	Length of I/O buffer.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 9-Jun-99 (gordy)
**	    Created.
*/

public void
setBuffSize( int size )
{
    /*
    ** The Ingres/Net Network Layer reserves 16 bytes
    ** at the start of the buffer for its own use.  The
    ** TCP-IP drivers only actually use 2 bytes, so we
    ** extend the buffer size by the lesser amount.
    */
    buf_max = size + 2;
    buffer = new byte[ buf_max ];
    clearBuffer();
    if ( trace.enabled( 5 ) )
	trace.write( title + ": set buffer size " + size );
    return;
} // setBuffSize

/*
** Name: clear
**
** Description:
**	Clear the I/O buffer.  Any existing data will be lost.
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
**	 9-Jun-99 (gordy)
**	    Created.
*/

public void
clear()
{
    clearBuffer();
    return;
} // clear

/*
** Name: close
**
** Description:
**	Close the associated I/O stream.
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
**	 9-Jun-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    A recursive loop can occur if a flush fails, which
**	    calls close() which calls flush(), etc.  Made sure
**	    close() won't recurse by setting the closed flag
**	    prior to calling flush().
*/

public synchronized void
close()
{
    if ( ! closed )
    {
	closed = true;

	if ( in != null )
	    try { in.close(); }
	    catch( Exception ignore ) {}

	if ( out != null )
	{
	    try 
	    {
		setSegSize();
		flushBuffer( true ); 
	    }
	    catch( Exception ignore ) {}
	    try { out.close(); }
	    catch( Exception ignore ) {}
	}

	clearBuffer();
	in = null;
	out = null;
	if ( trace.enabled( 3 ) )  trace.write( title + ": closed" );
    }

    return;
} // close

} // class IoBuff
