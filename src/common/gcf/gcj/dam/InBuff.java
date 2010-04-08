/*
** Copyright (c) 1999, 2003 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.dam;

/*
** Name: InBuff.java
**
** Description:
**	Defines class which supports buffered input from the server 
**	for the Data Access Messaging - Transport Layer protocol.
**
**  Classes
**
**	Inbuff		Extends IoBuff
**
** History:
**	16-Jun-99 (gordy)
**	    Created.
**	10-Sep-99 (gordy)
**	    Moved test for TL DR to receive().
**	13-Sep-99 (gordy)
**	    Implemented error code support.
**	22-Sep-99 (gordy)
**	    Added character set/encoding.
**	29-Sep-99 (gordy)
**	    Added the skip() method.
**	25-Jan-00 (gordy)
**	    Added some tracing.
**	28-Jan-00 (gordy)
**	    Added TL msg ID tracing.
**	20-Apr-00 (gordy)
**	    Moved to package io.
**	 6-Sep-02 (gordy)
**	    Character encoding now encapsulated in CharSet class.
**	 1-Oct-02 (gordy)
**	    Moved to GCF dam package.
**	31-Oct-02 (gordy)
**	    Renamed GCF error codes.
**	20-Dec-02 (gordy)
**	    Header ID now protocol level dependent.
**	22-Sep-03 (gordy)
**	    Support ByteArray values.
**	15-Mar-04 (gordy)
**	    Added support for eight-byte integers (Java long).
*/

import	java.io.InputStream;
import	java.sql.SQLException;
import	com.ingres.gcf.util.GcfErr;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.util.IdMap;
import	com.ingres.gcf.util.CharSet;
import	com.ingres.gcf.util.ByteArray;
import	com.ingres.gcf.util.Trace;
import	com.ingres.gcf.util.Tracing;
import	com.ingres.gcf.util.TraceLog;


/*
** Name: InBuff
**
** Description:
**	Implements buffered input from server corresponding to the
**	DAM-TL communications protocol.
**
**	The caller must provide multi-threading protection from the
**	invocation of the receive() method until the entire message
**	has been read.
**
**  Public Methods:
**
**	receive		Receive next message segment.
**	avail		Determine amount of data left in buffer.
**	readByte	Read a byte from the buffer/stream.
**	readShort	Read a short from the buffer/stream.
**	readInt		Read an integer from the buffer/stream.
**	readFloat	Read a float from the buffer/stream.
**	readDouble	Read a double from the buffer/stream.
**	readBytes	Read a byte array from the buffer/stream.
**	readString	Read a string from the buffer/stream.
**	skip		Skip bytes in the buffer/stream.
**
**  Protected Data
**
**	trace	    DAM-TL tracing
**
**  Private Methods:
**
**	need		Make sure sufficient data is present.
**
** History:
**	16-Jun-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Added the skip() method.
**	 1-Oct-02 (gordy)
**	    Added trace to override super-class to separate 
**	    Transport and Network Layer tracing.
**	22-Sep-03 (gordy)
**	    Added readBytes() varient to support ByteArray values.
**	15-Mar-04 (gordy)
**	    Added method to read long values (readLong()).
**      19-Nov-08 (rajus01) SIR 121238
**          Replaced SqlEx references with SQLException or SqlExFactory
**          depending upon the usage of it. SqlEx becomes obsolete to support
**          JDBC 4.0 SQLException hierarchy.
*/

class
InBuff
    extends IoBuff
    implements GcfErr
{

    protected Trace	    trace;	    // DAM-TL tracing (overrides IoBuff)


/*
** Name: InBuff
**
** Description:
**	Class constructor.
**
** Input:
**	in	Associated input stream.
**	id	Connection ID.
**	size	Length of input buffer.
**	log	Trace log.
**
** Output:
**	None.
**
** History:
**	16-Jun-99 (gordy)
**	    Created.
**	 1-Oct-02 (gordy)
**	    Added trace log parameter.  Initialize DAM-TL tracing.  
**	    Buffer size now set explicitly rather than super-class 
**	    initialization.
*/

public
InBuff( InputStream in, int id, int size, TraceLog log )
{
    super( in, log );
    title = "InBuff[" + id + "]";
    trace = new Tracing( log, DAM_TL_TRACE_ID );
    setBuffSize( size );
    return;
} // InBuff


/*
** Name: receive
**
** Description:
**	Read the next message.  Any data remaining in the current
**	message is discarded.  A server abort (TL disconnect request)
**	is processed here and results in an exception being raised.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	short	    TL Message type.
**
** History:
**	16-Jun-99 (gordy)
**	    Created.
**	10-Sep-99 (gordy)
**	    Check for TL disconnect request.
**	28-Jan-00 (gordy)
**	    Added TL msg ID tracing.
**	20-Dec-02 (gordy)
**	    Header ID now protocol level dependent.  Moved TL DR
**	    processing to MSG classes where it is more appropriatte.
*/

public short
receive()
    throws SQLException
{
    int	    hdr_id;
    short   msg_id;

    /*
    ** Get the next segment in the input buffer.
    ** If buffer is empty, receive next buffer.
    */
    super.next();

    /*
    ** Read and validate packet header: ID and type.
    */
    hdr_id = readInt();

    if ( 
	 (proto_lvl <= DAM_TL_PROTO_1  &&  hdr_id != DAM_TL_ID_1)  ||
	 (proto_lvl >= DAM_TL_PROTO_2  &&  hdr_id != DAM_TL_ID_2)
       )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write(title + ": invalid TL header ID");
	close();
	throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
    }

    msg_id = readShort();	// Packet type

    if ( trace.enabled( 2 ) )  
	trace.write(title + ": receive TL packet " + IdMap.map(msg_id, tlMap));
    if ( trace.enabled( 3 ) )
	trace.write(title + ": length " + avail() );

    return( msg_id );
} // receive


/*
** Name: avail
**
** Description:
**	Returns the amount of unread data available in the
**	current message segment.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Amount of data available.
**
** History:
**	16-Jun-99 (gordy)
**	    Created.
*/

public int
avail()
{
    return( data_end - data_ptr );
} // avail


/*
** Name: readByte
**
** Description:
**	Read a byte from the input stream
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	byte	    Value from input stream.
**
** History:
**	16-Jun-99 (gordy)
**	    Created.
*/

public byte
readByte()
    throws SQLException
{
    need( 1, true );
    return( buffer[ data_ptr++ ] );
} // readByte


/*
** Name: readShort
**
** Description:
**	Read a short value from the input buffer.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	short	    Value from input stream.
**
** History:
**	16-Jun-99 (gordy)
**	    Created.
*/

public short
readShort()
    throws SQLException
{
    need( 2, true );
    return( (short)((buffer[ data_ptr++ ] & 0x00ff) |
		    ((buffer[ data_ptr++ ] << 8) & 0xff00)) );
} // readShort


/*
** Name: readInt
**
** Description:
**	Read an integer value from the input buffer.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	    Value from input stream.
**
** History:
**	16-Jun-99 (gordy)
**	    Created.
*/

public int
readInt()
    throws SQLException
{
    need( 4, true );
    return( (int)(buffer[ data_ptr++ ] & 0x000000ff) |
		 ((buffer[ data_ptr++ ] << 8 ) & 0x0000ff00) |
		 ((buffer[ data_ptr++ ] << 16 ) & 0x00ff0000) |
		 ((buffer[ data_ptr++ ] << 24 ) & 0xff000000) );
} // readInt


/*
** Name: readLong
**
** Description:
**	Read a long value from the input buffer.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	long	    Value from input stream.
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
    return( (((long)buffer[ data_ptr++ ])       & 0x00000000000000ffL) |
	    (((long)buffer[ data_ptr++ ] << 8)  & 0x000000000000ff00L) |
	    (((long)buffer[ data_ptr++ ] << 16) & 0x0000000000ff0000L) |
	    (((long)buffer[ data_ptr++ ] << 24) & 0x00000000ff000000L) |
	    (((long)buffer[ data_ptr++ ] << 32) & 0x000000ff00000000L) |
	    (((long)buffer[ data_ptr++ ] << 40) & 0x0000ff0000000000L) |
	    (((long)buffer[ data_ptr++ ] << 48) & 0x00ff000000000000L) |
	    (((long)buffer[ data_ptr++ ] << 56) & 0xff00000000000000L) );
} // readLong


/*
** Name: readFloat
**
** Description:
**	Read a float value from the input buffer.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	float	    Value from input stream.
**
** History:
**	16-Jun-99 (gordy)
**	    Created.
*/

public float
readFloat()
    throws SQLException
{
    float   value = Float.NaN;
    int	    man;
    short   exp;
    byte    sign;
    
    /*
    ** Extract components from network format.
    */
    need( DAM_TL_F4_LEN, true );
    sign = (byte)((buffer[ data_ptr + 1 ] >>> 7) & 0x00000001);
    exp = (short)(((buffer[ data_ptr + 1 ] << 8)  & 0x00007f00) | 
		  ( buffer[ data_ptr + 0 ]        & 0x000000ff));
    man = ((buffer[ data_ptr + 3 ] << 24) & 0xff000000) | 
	  ((buffer[ data_ptr + 2 ] << 16) & 0x00ff0000) | 
	  ((buffer[ data_ptr + 5 ] << 8)  & 0x0000ff00) | 
	  ( buffer[ data_ptr + 4 ]        & 0x000000ff);
    data_ptr += DAM_TL_F4_LEN;

    /*
    ** Re-bias the exponent.
    */
    if ( exp != 0 )  exp += (126 - 16382);

    if ( exp < 0  ||  exp > 255 )
	value = (sign != 0) ? Float.MIN_VALUE : Float.MAX_VALUE;
    else
    {
	/*
	** Rebuild the double from the components.
	*/
	int bits = ((sign << 31) & 0x80000000) | 
		   ((exp << 23)  & 0x7f800000) |
		   ((man >>> 9)  & 0x007fffff);

	value = Float.intBitsToFloat( bits );
    }
    
    return( value );
} // readFloat


/*
** Name: readDouble
**
** Description:
**	Read a double value from the input buffer.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	double	    Value from input stream.
**
** History:
**	16-Jun-99 (gordy)
**	    Created.
*/

public double
readDouble()
    throws SQLException
{
    double  value = Double.NaN;
    long    man;
    short   exp;
    byte    sign;
    
    /*
    ** Extract components from network format.
    */
    need( DAM_TL_F8_LEN, true );
    sign = (byte)((buffer[ data_ptr + 1 ] >>> 7) & 0x00000001);
    exp = (short)(((buffer[ data_ptr + 1 ] << 8)  & 0x00007f00) | 
		  ( buffer[ data_ptr + 0 ]        & 0x000000ff));
    man = (((long)buffer[ data_ptr + 3 ] << 56) & 0xff00000000000000L) | 
	  (((long)buffer[ data_ptr + 2 ] << 48) & 0x00ff000000000000L) | 
	  (((long)buffer[ data_ptr + 5 ] << 40) & 0x0000ff0000000000L) | 
	  (((long)buffer[ data_ptr + 4 ] << 32) & 0x000000ff00000000L) |
	  (((long)buffer[ data_ptr + 7 ] << 24) & 0x00000000ff000000L) | 
	  (((long)buffer[ data_ptr + 6 ] << 16) & 0x0000000000ff0000L) |
	  (((long)buffer[ data_ptr + 9 ] << 8)  & 0x000000000000ff00L) | 
	  ( (long)buffer[ data_ptr + 8 ]        & 0x00000000000000ffL);
    data_ptr += DAM_TL_F8_LEN;

    /*
    ** Re-bias the exponent.
    */
    if ( exp != 0 )  exp += (1022 - 16382);

    if ( exp < 0  ||  exp > 2047 )
	value = (sign != 0) ? Double.MIN_VALUE : Double.MAX_VALUE;
    else
    {
	/*
	** Rebuild the double from the components.
	*/
	long bits = (((long)sign << 63) & 0x8000000000000000L) | 
		    (((long)exp << 52)  & 0x7ff0000000000000L) |
		    ((      man >>> 12) & 0x000fffffffffffffL);

	value = Double.longBitsToDouble( bits );
    }
    
    return( value );
} // readDouble


/*
** Name: readBytes
**
** Description:
**	Read a byte array from the input stream.  There must be
**	sufficient space in output buffer to receive the requested
**	data.  Actual number of bytes read is limited to contents
**	of current input buffer.
**
** Input:
**	offset	    Offset in byte array for data.
**	length	    Number of bytes to place in array.
**
** Output:
**	bytes	    Bytes read from input stream.
**
** Returns:
**	int	    Number of bytes actually read from input stream.
**
** History:
**	16-Jun-99 (gordy)
**	    Created.
**	22-Sep-99 (gordy)
**	    Caller must provide valid length (no more discard).
**	    Changed return semantics to permit processing of split
**	    arrays.
*/

public int
readBytes( byte bytes[], int offset, int length )
    throws SQLException
{
    need( length, false );	// Make sure some data is available.
    length = Math.min( length, avail() );
    System.arraycopy( buffer, data_ptr, bytes, offset, length );
    data_ptr += length;
    return( length );
} // readBytes


/*
** Name: readBytes
**
** Description:
**	Read a ByteArray from the input stream.  ByteArray limits
**	are ignored.  Any requested data which is available which
**	exceeds the array limit is read and discarded.  It is the
**	callers responsibility to ensure adaquate array limits 
**	and capacity.  Actual number of bytes read is limited to 
**	contents of current input buffer.
**
** Input:
**	length	    Number of bytes to place in array.
**
** Output:
**	bytes	    BytesArray filled from input stream
**
** Returns:
**	int	    Number of bytes actually read from input stream.
**
** History:
**	22-Sep-03 (gordy)
**	    Created.
*/

public int
readBytes( ByteArray bytes, int length )
    throws SQLException
{
    need( length, false );	// Make sure some data is available.
    length = Math.min( length, avail() );
    bytes.put( buffer, data_ptr, length );  // Array limit overruns ignored!
    data_ptr += length;
    return( length );
} // readBytes


/*
** Name: readString
**
** Description:
**	Read a string from the input stream.  The string is treated
**	as an atomic value and the entire requested length must be
**	available.  The input bytes are converted to Unicode using 
**	the character encoding provided.  Strings which are split
**	must be read as byte arrays.
**
** Input:
**	length	    Number of bytes in input string.
**	char_set    Character encoding for conversion
**
** Output:
**	None.
**
** Returns:
**	String	    String value from input stream.
**
** History:
**	16-Jun-99 (gordy)
**	    Created.
**	22-Sep-99 (gordy)
**	    Added character set/encoding.
**	 6-Sep-02 (gordy)
**	    Character encoding now encapsulated in CharSet class.
*/

public String
readString( int length, CharSet char_set )
    throws SQLException
{
    String str;
    need( length, true );

    try { str = char_set.getString( buffer, data_ptr, length ); }
    catch( Exception ex )
    {
        throw SqlExFactory.get( ERR_GC401E_CHAR_ENCODE );	// Should not happen!
    }
    
    data_ptr += length;
    return( str );
} // readString


/*
** Name: skip
**
** Description:
**	Skip bytes in the input stream.  Actual number of bytes 
**	skipped is limited to contents of current input buffer.
**
** Input:
**	length	    Number of bytes to skip.
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
    need( length, false );  // Make sure some data is available.
    length = Math.min( length, avail() );
    data_ptr += length;
    return( length );
} // skip


/*
** Name: need
**
** Description:
**	Determines if sufficient data is available to read an
**	object of a particular size.  Atomic objects may not be 
**	split across buffers.  Non-atomic objects may be split
**	and may need to be read piece by piece.
**
** Input:
**	size	    Amount of data required.
**	atomic	    True if data may not be split.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	16-Jun-99 (gordy)
**	    Created.
**	10-Sep-99 (gordy)
**	    Moved check for TL disconnect request to receive().
**	22-Sep-99 (gordy)
**	    Added atomic indicating to permit handling of split arrays.
**	25-Jan-00 (gordy)
**	    Added tracing.
*/

private void
need( int size, boolean atomic )
    throws SQLException
{
    int	    avail;

    while( (avail = avail()) < size )
    {
	short   tl_id;

	if ( avail > 0 )
	    if ( ! atomic )
	        break;
	    else
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( title + ": atomic value split (" + 
				 avail + "," + size + ")" );
		close();
		throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	    }

	if ( (tl_id = receive()) != DAM_TL_DT )
	{
	    if ( trace.enabled( 1 ) )  
		trace.write( title + ": invalid TL packet ID 0x" + 
			     Integer.toHexString( tl_id ) );
	    close();
	    throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	}
    }

    if ( trace.enabled( 4 ) )
	if ( avail < size )
	    trace.write( title + ": reading " + 
			 avail + " (" + size + " requested)" );
	else
	    trace.write( title + ": reading " + size );

    return;
} // need


} // class InBuff
