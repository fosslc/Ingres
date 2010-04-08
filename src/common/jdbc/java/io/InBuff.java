/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.io;

/*
** Name: InBuff.java
**
** Description:
**	Driver class InBuff which supports buffered input from the server 
**	at the equivilent of the Ingres/Net Transport Layer level.
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
*/

import	java.io.InputStream;
import	ca.edbc.util.EdbcEx;
import	ca.edbc.util.IdMap;
import	ca.edbc.util.CharSet;


/*
** Name: InBuff
**
** Description:
**	Implements buffered input from server corresponding to the
**	Ingres/Net Transport Layer.
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
**  Private Methods:
**
**	need		Make sure sufficient data is present.
**
** History:
**	16-Jun-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Added the skip() method.
*/

class
InBuff
    extends IoBuff
{

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
**
** Output:
**	None.
**
** History:
**	16-Jun-99 (gordy)
**	    Created.
*/

public
InBuff( InputStream in, int id, int size )
{
    super( in, size );
    title = "InBuff[" + id + "]";
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
*/

public short
receive()
    throws EdbcEx
{
    short msg_id;

    /*
    ** Get the next segment in the input buffer.
    ** If buffer is empty, receive next buffer.
    */
    super.next();

    if ( readInt() != JDBC_TL_ID )
    {
	if ( trace.enabled( 1 ) )  trace.write(title + ": invalid TL header ID");
	close();
	throw EdbcEx.get( E_IO0002_PROTOCOL_ERR );
    }

    /*
    ** Determine type of message.  A disconnect request is only
    ** issued by the server when aborting the connection.  Handle
    ** this exceptional case directly.
    */
    msg_id = readShort();
    if ( trace.enabled( 3 ) )  
	trace.write( title + ": recv TL msg " + IdMap.map( msg_id, tlMap ) +
		     " length " + avail() );

    if ( msg_id == JDBC_TL_DR )
    {
	/*
	** An error code may be provided as an optional
	** parameter to the disconnect request.
	*/
	if ( avail() >= 2 )
	{
	    byte    param_id = readByte();
	    byte    param_len = readByte();
	    
	    if ( param_len == 4  &&  avail() >= 4 )  
	    {
		int error = readInt();

		if ( trace.enabled( 1 ) )  
		    trace.write( title + ": server abort 0x" + 
				 Integer.toHexString( error ) );
		// TODO: send TL_DC
		close();
		throw EdbcEx.get( E_IO0005_CONNECT_FAIL,
				  new EdbcEx( "Server requested disconnect.", 
					      "40003",  error) );
	    }
	}

	/*
	** Server failed to provided an abort code.
	*/
	if ( trace.enabled( 1 ) )  
	    trace.write( title + ": server Disconnect Request" );
	// TODO: send TL_DC
	close();
	throw EdbcEx.get( E_IO0005_CONNECT_FAIL );
    }
  
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
    throws EdbcEx
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
    throws EdbcEx
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
    throws EdbcEx
{
    need( 4, true );
    return( (int)(buffer[ data_ptr++ ] & 0x000000ff) |
		 ((buffer[ data_ptr++ ] << 8 ) & 0x0000ff00) |
		 ((buffer[ data_ptr++ ] << 16 ) & 0x00ff0000) |
		 ((buffer[ data_ptr++ ] << 24 ) & 0xff000000) );
} // readInt

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
    throws EdbcEx
{
    float   value = Float.NaN;
    int	    man;
    short   exp;
    byte    sign;
    
    /*
    ** Extract components from network format.
    */
    need( JDBC_TL_F4_LEN, true );
    sign = (byte)((buffer[ data_ptr + 1 ] >>> 7) & 0x00000001);
    exp = (short)(((buffer[ data_ptr + 1 ] << 8)  & 0x00007f00) | 
		  ( buffer[ data_ptr + 0 ]        & 0x000000ff));
    man = ((buffer[ data_ptr + 3 ] << 24) & 0xff000000) | 
	  ((buffer[ data_ptr + 2 ] << 16) & 0x00ff0000) | 
	  ((buffer[ data_ptr + 5 ] << 8)  & 0x0000ff00) | 
	  ( buffer[ data_ptr + 4 ]        & 0x000000ff);
    data_ptr += JDBC_TL_F4_LEN;

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
    throws EdbcEx
{
    double  value = Double.NaN;
    long    man;
    short   exp;
    byte    sign;
    
    /*
    ** Extract components from network format.
    */
    need( JDBC_TL_F8_LEN, true );
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
    data_ptr += JDBC_TL_F8_LEN;

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
**	int	    Number of bytes actually read.
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
    throws EdbcEx
{
    need( length, false );  // Make sure some data is available.
    length = Math.min( length, avail() );

    try { System.arraycopy( buffer, data_ptr, bytes, offset, length ); }
    catch( Exception ex ) 
    {
	// Should not happen!
	throw new EdbcEx( title + ": array copy exception" ); 
    }
    
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
    throws EdbcEx
{
    String str;
    need( length, true );

    try { str = char_set.getString( buffer, data_ptr, length ); }
    catch( Exception e ) 
    {
        // Should not happen!
        throw new EdbcEx( title + ": character encoding failed" );
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
    throws EdbcEx
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
    throws EdbcEx
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
		throw EdbcEx.get( E_IO0002_PROTOCOL_ERR );
	    }

	if ( (tl_id = receive()) != JDBC_TL_DT )
	{
	    if ( trace.enabled( 1 ) )  
		trace.write( title + ": invalid TL packet ID 0x" + 
			     Integer.toHexString( tl_id ) );
	    close();
	    throw EdbcEx.get( E_IO0002_PROTOCOL_ERR );
	}
    }

    if ( trace.enabled( 5 ) )
	if ( avail < size )
	    trace.write( title + ": reading " + 
			 avail + " (" + size + " requested)" );
	else
	    trace.write( title + ": reading " + size );

    return;
} // need

} // class InBuff
