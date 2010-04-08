/*
** Copyright (c) 2003, 2007 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.util;

/*
** Name: SqlLongByte.java
**
** Description:
**	Defines class which represents an SQL Long Binary value.
**
**  Classes:
**
**	SqlLongByte	An SQL Long Binary value.
**	BinIS2HexRdr	Convert binary InputStream to Hex Reader.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Added support for parameter types/values in addition to 
**	    existing support for columns.
**	15-Nov-06 (gordy)
**	    Enhance conversion support.
**	30-Jan-07 (gordy)
**	    Additional conversions enhancements.
**	26-Feb-07 (gordy)
**	    Enhanced with Blob compatibility.
**      17-Nov-08 (rajus01) SIR 121238
**          Replaced SqlEx references with SQLException or SqlExFactory
**          depending upon the usage of it. SqlEx becomes obsolete to support
**          JDBC 4.0 SQLException hierarchy.
*/

import	java.io.InputStream;
import	java.io.OutputStream;
import	java.io.Reader;
import	java.io.IOException;
import	java.sql.Blob;
import	java.sql.SQLException;


/*
** Name: SqlLongByte
**
** Description:
**	Class which represents an SQL Long Binary value.  SQL Long 
**	Binary values are potentially large variable length streams.
**
**	Supports conversion to String, Binary, Object and character
**	streams.  
**
**	The data value accessor methods do not check for the NULL
**	condition.  The super-class isNull() method should first
**	be checked prior to calling any of the accessor methods.
**
**  Inherited Methods:
**
**	setNull			Set a NULL data value.
**	toString		String representation of data value.
**	closeStream		Close the current stream.
**
**  Public Methods:
**
**	set			Assign a new data value.
**	get			Retrieve current data value.
**
**	setString		Data value accessor assignment methods
**	setBytes
**	setBinaryStream
**
**	getString		Data value accessor retrieval methods
**	getBytes
**	getBinaryStream
**	getAsciiStream
**	getUnicodeStream
**	getCharacterStream
**	getBlob
**	getObject
**
**	strm2array		Convert binary stream to byte array.
**	byteIS2hexRdr		Convert InputStream to Reader.
**
**  Protected Methods:
**
**	cnvtIS2Rdr		Convert InputStream to Reader.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Added parameter data value oriented methods.
**	15-Nov-06 (gordy)
**	    Made static method strm2array() public.  Added implementation
**	    which doesn't limit array length.
**	30-Jan-07 (gordy)
**	    Added byteIS2hexRdr.
**	26-Feb-07 (gordy)
**	    Added getBlob().
*/

public class
SqlLongByte
    extends SqlStream
{


/*
** Name: SqlLongByte
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

public
SqlLongByte()
{
    super();
} // SqlLongByte


/*
** Name: SqlLongByte
**
** Description:
**	Class constructor.  Data value is initially NULL.
**	Defines a SqlStream event listener for stream 
**	closure event notification.
**
** Input:
**	listener	Stream listener.
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
SqlLongByte( StreamListener listener )
{
    super( listener );
} // SqlLongByte


/*
** Name: set
**
** Description:
**	Assign a new data value.  The data value will be NULL if 
**	the input value is null, otherwise non-NULL.  The input 
**	stream may optionally implement the SqlStream.StreamSource
**	interface.
**
** Input:
**	stream		The new data value.
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

public void
set( InputStream stream )
    throws SQLException
{
    setStream( stream );
    return;
} // set


/*
** Name: set
**
** Description:
**	Assign a new data value as a copy of a SqlByte data 
**	value.  The data value will be NULL if the input value 
**	is null, otherwise non-NULL.  
**
** Input:
**	data	SqlByte data value to copy.
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
set( SqlByte data )
    throws SQLException
{
    if ( data == null  ||  data.isNull() )
	setNull();
    else
    {
	/*
	** The binary data is stored in a byte array.  A simple 
	** binary stream will produce the desired output.  Note 
	** that we need to follow the SqlByte convention and 
	** extend the data to the optional limit.
	*/
	data.extend();
	setStream( getBinary( data.value, 0, data.length ) );
    }
    return;
} // set


/*
** Name: set
**
** Description:
**	Assign a new data value as a copy of a SqlVarByte data 
**	value.  The data value will be NULL if the input value 
**	is null, otherwise non-NULL.  
**
** Input:
**	data	SqlVarByte data value to copy.
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
set( SqlVarByte data )
    throws SQLException
{
    if ( data == null  ||  data.isNull() )
	setNull();
    else
    {
	/*
	** The binary data is stored in a byte array.  A simple 
	** binary stream will produce the desired output.
	*/
	setStream( getBinary( data.value, 0, data.length ) );
    }
    return;
} // set


/*
** Name: get
**
** Description:
**	Return the current data value.
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
**	InputStream	Current value.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public InputStream
get()
    throws SQLException
{
    return( (InputStream)getStream() );
} // get


/*
** Name: get
**
** Description:
**	Write the current data value to an OutputStream.
**	The current data value is consumed.  The Output-
**	Stream is not closed.
**
**	Note: the value returned when the data value is 
**	NULL is not well defined.
**
** Input:
**	os	OutputStream to receive data value.
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
get( OutputStream os )
    throws SQLException
{
    copyIs2Os( (InputStream)getStream(), os );
    return;
} // get


/*
** Name: setString
**
** Description:
**	Assign a String value as the data value.
**	The data value will be NULL if the input 
**	value is null, otherwise non-NULL.
**
** Input:
**	value	String value (may be null).
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
setString( String value ) 
    throws SQLException
{
    if ( value == null )
	setNull();
    else
	setBytes( value.getBytes() );
    return;
} // setString


/*
** Name: setBytes
**
** Description:
**	Assign a byte array as the data value.
**	The data value will be NULL if the input 
**	value is null, otherwise non-NULL.
**
** Input:
**	value	Byte array (may be null).
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
setBytes( byte value[] ) 
    throws SQLException
{
    if ( value == null )
	setNull();
    else
	setStream( getBinary( value, 0, value.length ) );
    return;
} // setBytes


/*
** Name: setBinaryStream
**
** Description:
**	Assign a byte stream as the data value.
**	The data value will be NULL if the input 
**	value is null, otherwise non-NULL.
**
** Input:
**	value	Byte stream (may be null).
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
setBinaryStream( InputStream value ) 
    throws SQLException
{
    setStream( value );
    return;
} // setBinaryStream


/*
** Name: getString
**
** Description:
**	Return the current data value as a String value.
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
**	String	Current value converted to String.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
*/

public String 
getString() 
    throws SQLException
{
    byte ba[] = strm2array( getBinary() );
    return( SqlVarByte.bin2str( ba, 0, ba.length ) );
} // getString


/*
** Name: getString
**
** Description:
**	Return the current data value as a String value
**	with a maximum size limit.
**
**	Note: the value returned when the data value is 
**	NULL is not well defined.
**
** Input:
**	limit	Maximum length of result.	
**
** Output:
**	None.
**
** Returns:
**	String	Current value converted to String.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
*/

public String 
getString( int limit ) 
    throws SQLException
{
    byte ba[] = strm2array( getBinary(), limit );
    return( SqlVarByte.bin2str( ba, 0, ba.length ) );
} // getString


/*
** Name: getBytes
**
** Description:
**	Return the current data value as a byte array.
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
**	byte[]	Current value converted to byte array.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
*/

public byte[] 
getBytes() 
    throws SQLException
{
    return( strm2array( getBinary() ) );
} // getBytes


/*
** Name: getBytes
**
** Description:
**	Return the current data value as a byte array
**	with a maximum size limit.
**
**	Note: the value returned when the data value is 
**	NULL is not well defined.
**
** Input:
**	limit	Maximum length of result.	
**
** Output:
**	None.
**
** Returns:
**	byte[]	Current value converted to byte array.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
*/

public byte[]
getBytes( int limit )
    throws SQLException
{
    return( strm2array( getBinary(), limit ) );
} // getBytes


/*
** Name: getBinaryStream
**
** Description:
**	Return the current data value as a binary stream.
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
**	InputStream	Current value converted to binary stream.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
*/

public InputStream 
getBinaryStream() 
    throws SQLException
{ 
    return( getBinary() );
} // getBinaryStream


/*
** Name: getAsciiStream
**
** Description:
**	Return the current data value as an ASCII stream.
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
**	InputStream	Current value converted to ASCII stream.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
*/

public InputStream 
getAsciiStream() 
    throws SQLException
{ 
    return( getAscii() );
} // getAsciiStream


/*
** Name: getUnicodeStream
**
** Description:
**	Return the current data value as a Unicode stream.
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
**	InputStream	Current value converted to Unicode stream.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
*/

public InputStream 
getUnicodeStream() throws SQLException
{ 
    return( getUnicode() );
} // getUnicodeStream


/*
** Name: getCharacterStream
**
** Description:
**	Return the current data value as a character stream.
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
**	Reader	Current value converted to character stream.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
*/

public Reader 
getCharacterStream() 
    throws SQLException
{ 
    return( getCharacter() );
} // getCharacterStream


/*
** Name: getBlob
**
** Description:
**      Returns the current data value buffered in a Blob wrapper.
**
**      Note: the value returned when the data value is
**      NULL is not well defined.
**
** Input:
**      None.
**
** Output:
**      None.
**
** Returns:
**      Blob    Current value buffered and wrapped as a Blob.
**
** History:
**      26-Feb-07 (gordy)
**          Created.
*/

public Blob
getBlob()
    throws SQLException
{ 
    return( new BufferedBlob( getBinary() ) );
}


/*
** Name: getObject
**
** Description:
**	Return the current data value as an Binary object.
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
**	Object	Current value converted to Binary.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
*/

public Object 
getObject() 
    throws SQLException
{
    return( getBytes() );
} // getObject


/*
** Name: getObject
**
** Description:
**	Return the current data value as an Binary object
**	with a maximum size limit.
**
**	Note: the value returned when the data value is 
**	NULL is not well defined.
**
** Input:
**	limit	Maximum length of result.	
**
** Output:
**	None.
**
** Returns:
**	Object	Current value converted to Binary.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
*/

public Object 
getObject( int limit ) 
    throws SQLException
{
    return( getBytes( limit ) );
} // getObject


/*
** Name: cnvtIS2Rdr
**
** Description:
**	Converts the binary input stream into a Reader
**	stream by converting to hex characters.
**
** Input:
**	stream	    Input stream.
**
** Output:
**	None.
**
** Returns:
**	Reader		Converted Reader stream.
**
** History:
**	12-Sep-03 (gordy)
*/

protected Reader
cnvtIS2Rdr( InputStream stream )
    throws SQLException
{
    return( new BinIS2HexRdr( stream ) );
} // cnvtIS2Rdr


/*
** Name: byteIS2hexRdr
**
** Description:
**	Converts a binary input stream into a Reader
**	stream by converting to hex characters.
**
** Input:
**	stream	Input stream.
**
** Output:
**	None.
**
** Returns:
**	Reader	Converted Reader stream.
**
** History:
**	30-Jan-07 (gordy)
**	    Created.
*/

public static Reader
byteIS2hexRdr( InputStream stream )
{
    return( new BinIS2HexRdr( stream ) );
} // byteIS2hexRdr


/*
** Name: strm2array
**
** Description:
**	Read a binary input stream and convert to a byte array.  
**	The stream is closed.
**
** Input:
**	in	Binary input stream.
**
** Output:
**	None.
**
** Returns:
**	byte[]	The stream as a byte array.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public static byte[]
strm2array( InputStream in )
    throws SQLException
{
    byte	buff[] = new byte[ 8192 ];	// TODO: make static?
    byte	ba[] = new byte[ 0 ];
    int		len;

    try
    {
	while( (len = in.read( buff, 0, buff.length )) >= 0 )
	    if ( len > 0 )
	    {
		byte tmp[] = new byte[ ba.length + len ];
		if ( ba.length > 0 )  
		    System.arraycopy( ba, 0, tmp, 0, ba.length );
		System.arraycopy( buff, 0, tmp, ba.length, len );
		ba = tmp;
	    }
    }
    catch( IOException ex )
    {
	throw SqlExFactory.get( ERR_GC4007_BLOB_IO );
    }
    finally
    {
	try { in.close(); }
	catch( IOException ignore ) {}
    }

    return( ba );
} // strm2array


/*
** Name: strm2array
**
** Description:
**	Read a binary input stream and convert to a byte array.  
**	The stream is closed.
**
** Input:
**	in	Binary input stream.
**	limit	Maximum length of result.
**
** Output:
**	None.
**
** Returns:
**	byte[]	The stream as a byte array.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	15-Nov-06 (gordy)
**	    Made public.  Limit must now be valid, method
**	    without limit should be called otherwise.
*/

public static byte[]
strm2array( InputStream in, int limit )
    throws SQLException
{
    byte	buff[] = new byte[ 8192 ];	// TODO: make static?
    byte	ba[] = new byte[ 0 ];
    int		len;

    try
    {
	while( ba.length < limit  &&
	       (len = in.read( buff, 0, buff.length )) >= 0 )
	{
	    len = Math.min( len, limit - ba.length );
	    byte tmp[] = new byte[ ba.length + len ];
	    if ( ba.length > 0 )  System.arraycopy( ba, 0, tmp, 0, ba.length );
	    if ( len > 0 )  System.arraycopy( buff, 0, tmp, ba.length, len );
	    ba = tmp;
	}
    }
    catch( IOException ex )
    {
	throw SqlExFactory.get( ERR_GC4007_BLOB_IO );
    }
    finally
    {
	try { in.close(); }
	catch( IOException ignore ) {}
    }

    return( ba );
} // strm2array


/*
** Name: BinIS2HexRdr
**
** Description:
**	Class which converts a binary InputStream into a hex
**	character Reader.
**
**	This class is NOT expected to be used much, so the
**	implementation is simplistic and inefficient.
**
**  Public Methods:
**
**	read			Read characters.
**	skip			Skip characters.
**	close			Close stream.
**
**  Private Data:
**
**	stream			Input stream.
**	next			Next buffered character.
**	avail			Is next character buffered?
**
** History:
**	12-Sep-03 (gordy)
*/

private static class
BinIS2HexRdr
    extends Reader
{

    private InputStream		stream = null;
    private char		next = 0;
    private boolean		avail = false;
    

/*
** Name: BinIS2HexRdr
**
** Description:
**	Class constructor.
**
** Input:
**	stream		Input stream.
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
BinIS2HexRdr( InputStream stream )
{
    this.stream = stream;
} // BinIS2HexRdr


/*
** Name: read
**
** Description:
**	Read next character.  Returns -1 if at end-of-input.
**
**	Reads bytes from input stream, converts to two character
**	hex string.  Alternates returning first/second hex char.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Next character (0-65535) or -1 at end-of-input.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
*/

public synchronized int
read()
    throws IOException
{
    /*
    ** Check to see if next character is buffered.
    */
    if ( avail )
    {
	avail = false;
	return( (int)next );
    }

    /*
    ** Read next byte and check for EOF.
    */
    int value = stream.read();
    if ( value == -1 )  return( -1 );
    
    /*
    ** Convert to hex (1 or 2 digits).
    ** Return first digit, buffer second.
    */
    String str = Integer.toHexString( value & 0xff );
    char current;
    
    if ( str.length() == 1 )
    {
	current = '0';
	next = str.charAt(0);
    }
    else
    {
	current = str.charAt(0);
	next = str.charAt(1);
    }
	
    avail = true;
    return( (int)current );
} // read


/*
** Name: read
**
** Description:
**	Read characters into an array.
**
** Input:
**	None.
**
** Output:
**	chars	Character array.
**
** Returns:
**	int	Number of characters read, or -1 at end-of-input.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
*/

public int
read( char chars[] )
    throws IOException
{
    return( read( chars, 0, chars.length ) );
} // read


/*
** Name: read
**
** Description:
**	Read characters into a sub-array.
**
** Input:
**	offset	Starting position.
**	length	Number of characters to read.
**
** Output:
**	chars	Character array.
**
** Returns:
**	int	Number of characters read, or -1 at end-of-input.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
*/

public int
read( char chars[], int offset, int length )
    throws IOException
{
    int	limit = offset + length;
    int start = offset;
    
    while( offset < limit )
    {
	int ch = read();	// Character or EOF (-1)
	
	if ( ch == -1 )  
	    if ( offset == start )
		return( -1 );	// No characters read.
	    else
		break;
	
	chars[ offset++ ] = (char)ch;
    }
    
    return( offset - start );
} // read


/*
** Name: skip
**
** Description:
**	Skip characters in stream.
**
** Input:
**	count	Number of characters to skip.
**
** Output:
**	None.
**
** Returns:
**	long	Number of characters skipped.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
*/

public long
skip( long count )
    throws IOException
{
    long start = count;
    while( count > 0  &&  read() != -1 )  count--;
    return( start - count );
} // skip


/*
** Name: close
**
** Description:
**	Close the stream.
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

public void
close()
    throws IOException
{
    stream.close();
    return;
} // close


} // class BinIS2HexRdr


} // class SqlLongByte
