/*
** Copyright (c) 1999, 2010 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.dam;

/*
** Name: IoBuff.java
**
** Description:
**	Defines a class which supports buffered client-server I/O 
**	for the Data Access Messaging - Network Layer protocol.
**
**  Classes
**
**	IoBuff
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
**	 1-Oct-02 (gordy)
**	    Moved to GCF dam package.
**	31-Oct-02 (gordy)
**	    Renamed GCF error codes.
**	20-Dec-02 (gordy)
**	    Track DAM-TL protocol level.
**      19-Nov-08 (rajus01) SIR 121238
**          Replaced SqlEx references with SQLException or SqlExFactory
**          depending upon the usage of it. SqlEx becomes obsolete to support
**          JDBC 4.0 SQLException hierarchy.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	19-May-10 (gordy)
**	    Add option to flush() to indicate if output should be forced.
*/

import	java.io.InputStream;
import	java.io.OutputStream;
import	java.sql.SQLException;
import	com.ingres.gcf.util.GcfErr;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.util.Trace;
import	com.ingres.gcf.util.Tracing;
import	com.ingres.gcf.util.TraceLog;


/*
** Name: IoBuff
**
** Description:
**	Implements buffered client-server I/O corresponding to the
**	DAM-NL communications protocol.  Block buffer operations are 
**	supported, but no direct buffer access is provided publically.  
**	Instances of this class support either input or output, but 
**	not both.  It is expected that sub-classes will be defined 
**	providing input or output access to the I/O buffer.
**
**	The DAM-NL protocol supports segmentation of the data stream.  
**	Data segments are exposed to sub-classes to support concat-
**	enation without requiring buffer copies.
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
**  Constants
**
**	DAM_TL_TRACE_ID	Transport Layer trace ID.
**	DAM_NL_TRACE_ID	Network Layer trace ID.
**
**  Public Methods:
**
**	setBuffSize	Set the size of the I/O buffer.
**	clear		Clear the I/O buffer.
**	close		Close the associated I/O stream.
**
**  Protected Data:
**
**	buffer		The I/O buffer.
**	data_beg	Start of current data.
**	data_ptr	Current I/O position.
**	data_end	End of data (input) or buffer (output).
**	proto_lvl	DAM-TL protocol level.
**	title		Class title for tracing.
**	trace		DAM-NL tracing.
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
**	 1-Oct-02 (gordy)
**	    Added trace ID constants, finalize() and a private
**	    constructor for common initialization.
**	20-Dec-02 (gordy)
**	    Track DAM-TL protocol level.  Added proto_lvl and setProtoLvl().
*/

class
IoBuff
    implements TlConst, GcfErr
{

    /*
    ** Constants
    */
    public static final String	DAM_TL_TRACE_ID	= "msg.tl"; // DAM-TL trace ID.
    public static final String	DAM_NL_TRACE_ID	= "msg.nl"; // DAM-NL trace ID.

    /*
    ** I/O buffer and data indices.
    */
    protected byte	    buffer[];		// IO buffer.
    protected int	    data_beg;		// Start of data.
    protected int	    data_ptr;		// Current data position.
    protected int	    data_end;		// End of data/buffer.
    protected byte	    proto_lvl = DAM_TL_PROTO_1;	// Protocol level.

    /*
    ** Tracing.
    */
    protected String	    title = "IoBuff";	// Object title for tracing.
    protected Trace	    trace;		// DAM-NL tracing.

    /*
    ** I/O streams.
    */
    private InputStream	    in = null;		// Input data stream.
    private OutputStream    out = null;		// Output data stream.
    private boolean	    closed = false;	// Is data stream closed?
    
    /*
    ** Local buffer indices.
    */
    private int		    buf_max;		// Buffer size.
    private int		    buf_len;		// End of data.
    private int		    seg_beg;		// Start of current segment.
    private int		    seg_end;		// End of current segment.


/*
** Name: IoBuff
**
** Description:
**	Class constructor for input buffers.  Sub-class must set
**	buffer size to initialize the input buffer.
**
** Input:
**	in	The associated input stream.
**	log	Trace log.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 9-Jun-99 (gordy)
**	    Created.
**	28-Mar-01 (gordy)
**	    Separated tracing interface and implementation.
**	 1-Oct-02 (gordy)
**	    Removed buffer size and added trace log.
*/

protected
IoBuff( InputStream in, TraceLog log )
{
    this( log );
    this.in = in;
    return;
} // IoBuff


/*
** Name: IoBuff
**
** Description:
**	Class constructor for output buffers.  Sub-class must set
**	buffer size to initialize the output buffer.
**
** Input:
**	out	The associated output stream.
**	log	Trace log.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 9-Jun-99 (gordy)
**	    Created.
**	28-Mar-01 (gordy)
**	    Separated tracing interface and implementation.
**	 1-Oct-02 (gordy)
**	    Removed buffer size and added trace log.
*/

protected
IoBuff( OutputStream out, TraceLog log )
{
    this( log );
    this.out = out;
    return;
} // IoBuff


/*
** Name: IoBuff
**
** Description:
**	Class constructor for common initialization.
**
** Input:
**	log	Trace log.
**
** Ouptut:
**	None.
**
** Returns:
**	None.
**
** History:
**	 1-Oct-02 (gordy)
**	    Created.
*/

private
IoBuff( TraceLog log )
{
    trace = new Tracing( log, DAM_NL_TRACE_ID );
    return;
} // IoBuff


/*  
** Name: finalize
**
** Description:
**	Class destructor.  Close connection if still open.
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
**	 1-Oct-02 (gordy)
**	    Created.
*/

protected void 
finalize() 
    throws Throwable 
{
    close();
    super.finalize();
    return;
} // finalize


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
	if ( trace.enabled( 2 ) )  trace.write( title + ": closed" );
    }

    return;
} // close

  
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
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Add synchronization protection.
*/

public synchronized void
clear()
{
    clearBuffer();
    return;
} // clear


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
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Add synchronization protection.
*/

public synchronized void
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
    if ( trace.enabled( 4 ) ) trace.write(title + ": set buffer size " + size);
    return;
} // setBuffSize


/*
** Name: setProtoLvl
**
** Description:
**	Set the DAM-TL protocol level.
**
** Input:
**	proto_lvl	DAM-TL protocol level.
**
** Output:
**	None.
**
** Returns:
**	byte		Previous protocol level.
**
** History:
**	20-Dec-02 (gordy)
**	    Created.
*/

public byte
setProtoLvl( byte proto_lvl )
{
    byte prev = this.proto_lvl;
    this.proto_lvl = proto_lvl;
    return( prev );
} // setProtoLvl


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
    throws SQLException
{
    if ( closed  ||  out == null )
	throw SqlExFactory.get( ERR_GC4004_CONNECTION_CLOSED );

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
**	Send output buffer to server with option to flush.
**
** Input:
**	force	Should buffer be flushed?
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
**	19-May-10 (gordy)
**	    Added flush indicator.
*/

protected synchronized void
flush( boolean force )
    throws SQLException
{
    if ( closed  ||  out == null )
	throw SqlExFactory.get( ERR_GC4004_CONNECTION_CLOSED );

    setSegSize();	// Update current segment length.
    flushBuffer( force );
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
    throws SQLException
{
    int size;

    if ( closed  ||  in == null )
	throw SqlExFactory.get( ERR_GC4004_CONNECTION_CLOSED );

    if ( seg_beg < seg_end )  seg_beg = seg_end;
    if ( seg_beg >= buf_len )  clearBuffer();

    fillBuffer( 2 );		// Make sure length is available
    size = getSegSize();
    
    if ( trace.enabled( 4 ) )  
	trace.write( title + ": get segment length " + size );

    fillBuffer( size );		// Make sure entire segment is available
    seg_end = seg_beg + size;
    data_beg = data_ptr = seg_beg + 2;
    data_end = seg_end;

    return;
} // next


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
    throws SQLException
{
    if ( buf_len <= 0 )  return;

    if ( trace.enabled( 2 ) )  
	trace.write( title + ": sending " + buf_len + " bytes." );
    if ( trace.enabled( 3 ) )  
	trace.hexdump( buffer, 0, buf_len );

    try { out.write( buffer, 0, buf_len ); }
    catch( Exception ex )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( title + ": write error: " + ex.getMessage() );
	clearBuffer();
	close();
	throw SqlExFactory.get( ERR_GC4003_CONNECT_FAIL, ex );
    }

    clearBuffer();

    if ( force )
	try { out.flush(); }
	catch( Exception ex )
	{
	    if ( trace.enabled( 1 ) )
		trace.write( title + ": flush error: " + ex.getMessage() );
	    close();
	    throw SqlExFactory.get( ERR_GC4003_CONNECT_FAIL, ex );
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
    throws SQLException
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
	    trace.write( title + ": buffer overflow (" + size + "," + 
			 buf_max + ")" );
	close();
	throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
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

	if ( trace.enabled( 4 ) )
	    trace.write( title + ": reading " + (buf_max - buf_len) + 
			 " bytes (" + size + " requested, " + buf_len + 
			 " buffered)" );

	try { length = in.read( buffer, buf_len, buf_max - buf_len ); }
	catch( Exception ex )
	{
	    if ( trace.enabled( 1 ) )  
		trace.write( title + ": read error: " + ex.getMessage() );
	    close();
	    throw SqlExFactory.get( ERR_GC4003_CONNECT_FAIL, ex );
	}

	if ( length < 0 )
	{
	    if ( trace.enabled( 1 ) )  
		trace.write( title + ": connection closed by server" );
	    close();
	    throw SqlExFactory.get( ERR_GC4003_CONNECT_FAIL );
	}

	if ( trace.enabled( 2 ) )  
	    trace.write( title + ": received " + length + " bytes." );
	if ( trace.enabled( 3 ) )
	    trace.hexdump( buffer, buf_len, length );

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
	if ( trace.enabled( 4 ) )  
	    trace.write( title + ": set segment length " + size );

	/*
	** The Ingres NL standard requires little endian
	** for the length indicator.
	*/
	buffer[ seg_beg ] = (byte)(size & 0xff);
	buffer[ seg_beg + 1 ] = (byte)((size >> 8) & 0xff);
    }

    return;
} // setSegSize


} // class IoBuff
