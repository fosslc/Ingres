/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.io;

/*
** Name: OutBuff.java
**
** Description:
**	EDBC Driver class OutBuff which supports buffered output 
**	to the server at the equivilent of the Ingres/Net Transport 
**	Layer level.
**
** History:
**	16-Jun-99 (gordy)
**	    Created.
**	13-Sep-99 (gordy)
**	    Implemented error code support.
**	22-Sep-99 (gordy)
**	    Removed write( String ) method since it did not handle
**	    character set/encoding.
**	29-Sep-99 (gordy)
**	    Added write( InputStream ) method to fill current output
**	    buffer from the input stream.
**	28-Jan-00 (gordy)
**	    Replaced redundant code with check() method.
**	    Added TL msg ID tracing.
**	20-Apr-00 (gordy)
**	    Moved to package io.
*/

import	java.io.OutputStream;
import	java.io.InputStream;
import	ca.edbc.util.EdbcEx;
import	ca.edbc.util.IdMap;


/*
** Name: OutBuff
**
** Description:
**	Implements buffered output to server corresponding to the
**	Ingres/Net Transport Layer.  The output may be treated as
**	a stream (internal buffering) or the output buffer may be
**	accessed directly.
**
**	A message is created by first invoking the begin() method,
**	followed by any number of calls to write the body of the
**	message, and finished by invoking the flush() method.  It
**	is the callers responsibility to provide multi-threaded
**	protection from just prior to invoking begin() until after
**	the invocation of flush().
**
**  Public Methods:
**
**	begin	    Start a new message.
**	avail	    Returns amount of remaining space in buffer.
**	position    Current position in output buffer.
**	write	    Append/write values to output buffer.
**	flush	    Send output buffer to server.
**
**  Private Methods:
**
**	need	    Ensure space in output buffer.
**
** History:
**	16-Jun-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Added write( InputStream ) method to fill current output
**	    buffer from the input stream.
*/

class
OutBuff
    extends IoBuff
{

/*
** Name: OutBuff
**
** Description:
**	Class constructor.
**
** Input:
**	out	Associated output stream.
**	id	Connection ID.
**	size	Length of output buffer.
**
** Output:
**	None.
**
** History:
**	16-Jun-99 (gordy)
**	    Created.
*/

public
OutBuff( OutputStream out, int id, int size )
{
    super( out, size );
    title = "OutBuff[" + id + "]";
} // OutBuff

/*
** Name: begin
**
** Description:
**	Start a new message of the requested type.
**
** Input:
**	type	Message type.
**	size	Minimum buffer space required for message.
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
**	28-Jan-00 (gordy)
**	    Added TL msg ID tracing.
*/

public void
begin( short type, int size )
    throws EdbcEx
{
    if ( trace.enabled( 3 ) )
    {
	trace.write( title + ": begin TL msg " + IdMap.map( type, tlMap ) );
	if ( trace.enabled( 5 ) )  
	    trace.write( title + ": reserving " + (size + 6) );
    }

    super.begin( size + 6 );
    write( (int)JDBC_TL_ID );	// TL header
    write( (short)type );
    return;
} // begin

/*
** Name: avail
**
** Description:
**	Determine amount of unused space in output buffer.
**	Information returned is only valid after calling
**	begin() and prior to calling flush().
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Amount of unused space.
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
** Name: position
**
** Description:
**	Returns the current position in the output buffer.
**	Position is only valid after calling begin() and
**	prior to calling flush().  An appending write()
**	call may also invalidate the position (if length 
**	written exceeds avail() amount).
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Current position.
**
** History:
**	16-Jun-99 (gordy)
**	    Created.
*/

public int
position()
{
    return( data_ptr );
} // position

/*
** Name: write
**
** Description:
**	Append a byte value in the output buffer.
**
** Input:
**	value	    Byte value.
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
*/

public void
write( byte value )
    throws EdbcEx
{
    need( 1 );
    write( data_ptr, value );
    return;
} // write

/*
** Name: write
**
** Description:
**	Write a byte value in the output buffer.
**
** Input:
**	position    Buffer position.
**	value	    Byte value.
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
**	28-Jan-00 (gordy)
**	    Replaced redundat code with check() method.
*/

public void
write( int position, byte value )
    throws EdbcEx
{
    check( position, 1 );
    buffer[ position++ ] = value;
    if ( position > data_ptr )  data_ptr = position;
    return;
} // write

/*
** Name: write
**
** Description:
**	Append a short value in the output buffer.
**
** Input:
**	value	    Short value.
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
*/

public void
write( short value )
    throws EdbcEx
{
    need( 2 );
    write( data_ptr, value );
    return;
} // write 

/*
** Name: write
**
** Description:
**	Write a short value in the output buffer.
**
** Input:
**	position    Buffer position.
**	value	    Short value.
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
**	28-Jan-00 (gordy)
**	    Replaced redundat code with check() method.
*/

public void
write( int position, short value )
    throws EdbcEx
{
    check( position, 2 );
    buffer[ position++ ] = (byte)(value & 0xff);
    buffer[ position++ ] = (byte)((value >>> 8) & 0xff);
    if ( position > data_ptr )  data_ptr = position;
    return;
} // write

/*
** Name: write
**
** Description:
**	Append an integer value in the output buffer.
**
** Input:
**	value	    Integer value.
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
*/

public void
write( int value )
    throws EdbcEx
{
    need( 4 );
    write( data_ptr, value );
    return;
} // write

/*
** Name: write
**
** Description:
**	Write an integer value in the output buffer.
**
** Input:
**	position    Buffer position.
**	value	    Integer value.
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
**	28-Jan-00 (gordy)
**	    Replaced redundat code with check() method.
*/

public void
write( int position, int value )
    throws EdbcEx
{
    check( position, 4 );
    buffer[ position++ ] = (byte)(value & 0xff);
    buffer[ position++ ] = (byte)((value >>> 8 ) & 0xff);
    buffer[ position++ ] = (byte)((value >>> 16 ) & 0xff);
    buffer[ position++ ] = (byte)((value >>> 24 ) & 0xff);
    if ( position > data_ptr )  data_ptr = position;
    return;
} // write

/*
** Name: write
**
** Description:
**	Append a float value in the output buffer.
**
** Input:
**	value	    Float value.
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
*/

public void
write( float value )
    throws EdbcEx
{
    need( JDBC_TL_F4_LEN );
    write( data_ptr, value );
    return;
} // write

/*
** Name: write
**
** Description:
**	Write a float value in the output buffer.
**
** Input:
**	position    Buffer position.
**	value	    Float value.
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
**	28-Jan-00 (gordy)
**	    Replaced redundat code with check() method.
*/

public void
write( int position, float value )
    throws EdbcEx
{
    int	    bits, man;
    short   exp;
    byte    sign;

    check( position, JDBC_TL_F4_LEN );

    /*
    ** Extract bits of each component.
    */
    bits = Float.floatToIntBits( value );
    sign = (byte)((bits & 0x80000000) >>> 31);
    exp =  (short)((bits & 0x7f800000) >>> 23);
    man =  (int)((bits & 0x007fffff) << 9);

    /*
    ** Re-bias the exponent
    */
    if ( exp != 0 )  exp -= (126 - 16382);

    /*
    ** Pack the bits in network format.
    ** (note that byte pairs are swapped).
    */
    buffer[ position + 1 ] = (byte)((sign << 7) | (exp >>> 8));
    buffer[ position + 0 ] = (byte)exp;
    buffer[ position + 3 ] = (byte)(man >>> 24);
    buffer[ position + 2 ] = (byte)(man >>> 16);
    buffer[ position + 5 ] = (byte)(man >>> 8);
    buffer[ position + 4 ] = (byte)man;
    position += JDBC_TL_F4_LEN;
    if ( position > data_ptr )  data_ptr = position;

    return;
} // write

/*
** Name: write
**
** Description:
**	Append a double value in the output buffer.
**
** Input:
**	value	    Double value.
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
*/

public void
write( double value )
    throws EdbcEx
{
    need( JDBC_TL_F8_LEN );
    write( data_ptr, value );
    return;
} // write

/*
** Name: write
**
** Description:
**	Write a double value in the output buffer.
**
** Input:
**	position    Buffer position.
**	value	    Double value.
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
**	28-Jan-00 (gordy)
**	    Replaced redundat code with check() method.
*/

public void
write( int position, double value )
    throws EdbcEx
{
    long    bits, man;
    short   exp;
    byte    sign;

    check( position, JDBC_TL_F8_LEN );

    /*
    ** Extract bits of each component.
    */
    bits = Double.doubleToLongBits( value );
    sign = (byte)((bits & 0x8000000000000000L) >>> 63);
    exp =  (short)((bits & 0x7ff0000000000000L) >>> 52);
    man =  (bits & 0x000fffffffffffffL) << 12;

    /*
    ** Re-bias the exponent
    */
    if ( exp != 0 )  exp -= (1022 - 16382);

    /*
    ** Pack the bits in network format.
    ** (note that byte pairs are swapped).
    */
    buffer[ position + 1 ] = (byte)((sign << 7) | (exp >>> 8));
    buffer[ position + 0 ] = (byte)exp;
    buffer[ position + 3 ] = (byte)(man >>> 56);
    buffer[ position + 2 ] = (byte)(man >>> 48);
    buffer[ position + 5 ] = (byte)(man >>> 40);
    buffer[ position + 4 ] = (byte)(man >>> 32);
    buffer[ position + 7 ] = (byte)(man >>> 24);
    buffer[ position + 6 ] = (byte)(man >>> 16);
    buffer[ position + 9 ] = (byte)(man >>> 8);
    buffer[ position + 8 ] = (byte)man;
    position += JDBC_TL_F8_LEN;
    if ( position > data_ptr )  data_ptr = position;

    return;
} // write

/*
** Name: write
**
** Description:
**	Append a byte array to the output buffer.  This
**	routine does not split arrays across buffers, the
**	current buffer is flushed and the array must fit
**	in the new buffer, or an exception is thrown.
**
** Input:
**	value	    Byte array.
**	offset	    Position of data in array.
**	length	    Length of data in array.
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
*/

public void
write( byte value[], int offset, int length )
    throws EdbcEx
{
    if ( ! need( length ) )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( title + ": array length " + length + " too long" );
	close();
	throw EdbcEx.get( E_IO0002_PROTOCOL_ERR );
    }

    try { System.arraycopy( value, offset, buffer, data_ptr, length ); }
    catch( Exception e )
    {
	// Should not happen!
	throw new EdbcEx( title + ": array copy exception" );
    }

    data_ptr += length;
    if ( trace.enabled( 5 ) )  trace.write( title + ": appending " + length );
    return;
} // write

/*
** Name: write
**
** Description:
**	Append input stream to the output buffer.  This
**	routine does not split buffers, the input stream
**	is read until the buffer is filled or the stream
**	is exhausted.  The number of bytes read from the 
**	input stream is returned.  It is the callers
**	responsibility to assure that there is space
**	available in the output buffer.
**
** Input:
**	in	InputStream
**
** Output:
**	None.
**
** Returns:
**	short	Number of bytes written.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
*/

public short
write( InputStream in )
    throws EdbcEx
{
    int length = avail();

    try { length = in.read( buffer, data_ptr, length ); }
    catch( Exception ex )
    {
	throw EdbcEx.get( E_IO0007_BLOB_IO, ex );
    }

    if ( length < 0 )  length = 0;
    data_ptr += length;
    if ( trace.enabled( 5 ) )  trace.write( title + ": appending " + length );
    return( (short)length );
} // write

/*
** Name: flush
**
** Description:
**	Send buffer to server.
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
**	16-Jun-99 (gordy)
**	    Created.
*/

public void
flush()
    throws EdbcEx
{
    super.flush();
    return;
} // flush

/*
** Name: need
**
** Description:
**	Make sure enough room is available in the output
**	buffer for an object of the requested size.  If 
**	the current buffer is full, it will be flushed and 
**	a new DATA message started.
**
** Input:
**	size	    Amount of space required.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if room is available, false otherwise.
**
** History:
**	16-Jun-99 (gordy)
**	    Created.
*/

private boolean
need( int size )
    throws EdbcEx
{
    if ( (data_ptr + size) > data_end )
    {
	super.flush();
	begin( JDBC_TL_DT, size );
    }

    return( (data_ptr + size) <= data_end );
} // need

/*
** Name: check
**
** Description:
**	Validate a buffer write request.
**
** Input:
**	position:	Position in buffer.
**	length:		Length of data.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	28-Jan-00 (gordy)
**	    Created to replace redundant code and provide tracing.
*/

private void
check( int position, int size )
    throws EdbcEx
{
    if ( position < data_beg  ||  (position + size) > data_end )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( title + ": position out-of-bounds" );
	close();
	throw EdbcEx.get( E_IO0002_PROTOCOL_ERR );
    }
    else  if ( trace.enabled( 5 ) )
    {
	if ( position != data_ptr )
	    trace.write( title + ": writing " + size + " @ " + position );
	else
	    trace.write( title + ": appending " + size );
    }

    return;
}

} // class OutBuff
