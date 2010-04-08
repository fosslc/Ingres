/*
** Copyright (c) 2007 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.util;

/*
** Name: SqlLongByteCache.java
**
** Description:
**	Defines class which represents an SQL Cached Long Binary value.
**
**  Classes:
**
**	SqlLongByteCache	An SQL Cached Long Binary value.
**
** History:
**	26-Feb-07 (gordy)
**	    Created.
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
import  java.sql.SQLException;


/*
** Name: SqlLongByteCache
**
** Description:
**	Class which represents an SQL Cached Long Binary value.  
**	SQL Long Binary values are potentially large variable length 
**	streams.
**
**	Supports conversion to String, Binary, Object and character
**	streams.  
**
**	The data value accessor methods do not check for the NULL
**	condition.  The super-class isNull() method should first
**	be checked prior to calling any of the accessor methods.
**
**  Public Methods:
**
**	set			Assign a new data value.
**	get			Retrieve current data value.
**	setNull			Set a NULL data value.
**	toString		String representation of data value.
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
**  Private Data:
**
**	buffer			Buffered data value.
**	segSize			Buffer segment size.
**
** History:
**	26-Feb-07 (gordy)
**	    Created.
*/

public class
SqlLongByteCache
    extends SqlData
{

    private ByteBuffer		buffer = null;
    private int			segSize = 8192;


/*
** Name: SqlLongByteCache
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public
SqlLongByteCache()
{
    super( true );
} // SqlLongByteCache


/*
** Name: SqlLongByteCache
**
** Description:
**	Class constructor.  Data value is initially NULL.
**
** Input:
**	segSize	Segment size.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	26-Feb-07 (gordy)
**	    Created.
*/

public
SqlLongByteCache( int segSize )
{
    super( true );
    this.segSize = segSize;
} // SqlLongByteCache


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
**	26-Feb-07 (gordy)
**	    Created.
*/

public void
setNull()
{
    super.setNull();
    buffer = null;	// Free resources
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
**	long values.  Override to simply call the toString()
**	method of the current cache (watching out for NULL
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public String
toString()
{
    return( "SqlCache: " + ((buffer == null) ? "NULL" : buffer.toString()) );
} // toString


/*
** Name: set
**
** Description:
**	Assign a new data value.  The data value will be NULL if 
**	the input value is null, otherwise non-NULL.
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public void
set( InputStream stream )
    throws SQLException
{
    if ( stream == null )
	setNull();
    else
    {
	/*
	** A new buffer is created for each new value because
	** the previous value may be referenced from an external
	** object returned by one of the accessor methods.
	*/
	setNotNull();
	buffer = new ByteBuffer( segSize, stream );
    }
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
**	26-Feb-07 (gordy)
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
	** A new buffer is created for each new value because
	** the previous value may be referenced from an external
	** object returned by one of the accessor methods.
	**
	** The binary data is stored in a byte array.  Note 
	** that we need to follow the SqlByte convention and 
	** extend the data to the optional limit.
	*/
	setNotNull();
	data.extend();
	buffer = new ByteBuffer( segSize, data.value, 0, data.length );
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
**	26-Feb-07 (gordy)
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
	** A new buffer is created for each new value because
	** the previous value may be referenced from an external
	** object returned by one of the accessor methods.
	**
	** The binary data is stored in a byte array.  
	*/
	setNotNull();
	buffer = new ByteBuffer( segSize, data.value, 0, data.length );
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public InputStream
get()
    throws SQLException
{
    return( buffer.getIS() );
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public void
get( OutputStream os )
    throws SQLException
{
    buffer.read( os );
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
**	26-Feb-07 (gordy)
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public void 
setBytes( byte value[] ) 
    throws SQLException
{
    if ( value == null )
	setNull();
    else
    {
	/*
	** A new buffer is created for each new value because
	** the previous value may be referenced from an external
	** object returned by one of the accessor methods.
	*/
	setNotNull();
	buffer = new ByteBuffer( segSize, value, 0, value.length );
    }
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public void 
setBinaryStream( InputStream value ) 
    throws SQLException
{
    if ( value == null )
	setNull();
    else
    {
	/*
	** A new buffer is created for each new value because
	** the previous value may be referenced from an external
	** object returned by one of the accessor methods.
	*/
	setNotNull();
	buffer = new ByteBuffer( segSize, value );
    }
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public String 
getString() 
    throws SQLException
{
    byte bytes[] = getBytes();
    return( SqlVarByte.bin2str( bytes, 0, bytes.length ) );
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public String 
getString( int limit ) 
    throws SQLException
{
    byte bytes[] = getBytes( limit );
    return( SqlVarByte.bin2str( bytes, 0, bytes.length ) );
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public byte[] 
getBytes() 
    throws SQLException
{
    /*
    ** There is a natural limit at the max integer length.
    */
    return( getBytes( Integer.MAX_VALUE ) );
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public byte[]
getBytes( int limit )
    throws SQLException
{
    limit = (int)Math.min( (long)limit, buffer.length() );
    byte bytes[] = new byte[ limit ];
    buffer.read( 0, bytes, 0, limit );
    return( bytes );
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public InputStream 
getBinaryStream() 
    throws SQLException
{ 
    return( buffer.getIS() );
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public InputStream 
getAsciiStream() 
    throws SQLException
{ 
    return( SqlStream.getAsciiIS( 
		SqlLongByte.byteIS2hexRdr( buffer.getIS() ) ) );
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public InputStream 
getUnicodeStream() throws SQLException
{ 
    return( SqlStream.getUnicodeIS( 
		SqlLongByte.byteIS2hexRdr( buffer.getIS() ) ) );
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public Reader 
getCharacterStream() 
    throws SQLException
{ 
    return( SqlLongByte.byteIS2hexRdr( buffer.getIS() ) );
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
    return( new BufferedBlob( buffer ) );
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
**	26-Feb-07 (gordy)
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public Object 
getObject( int limit ) 
    throws SQLException
{
    return( getBytes( limit ) );
} // getObject


} // class SqlLongByteCache
