/*
** Copyright (c) 2003, 2008 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.util;

/*
** Name: SqlLongChar.java
**
** Description:
**	Defines class which represents an SQL LongVarchar value.
**
**  Classes:
**
**	SqlLongChar	An SQL LongVarchar value.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Added support for parameter types/values in addition to 
**	    existing support for columns.
**	15-Nov-06 (gordy)
**	    Enhance conversion support.
**      17-Nov-08 (rajus01) SIR 121238
**          Replaced SqlEx references with SQLException or SqlExFactory
**          depending upon the usage of it. SqlEx becomes obsolete to support
**          JDBC 4.0 SQLException hierarchy.
**	19-Dec-08 (gordy)
**	    Allow getBytes() and getBinaryStream().
*/

import	java.io.InputStream;
import	java.io.OutputStream;
import	java.io.Reader;
import	java.io.Writer;
import	java.sql.SQLException;
import  com.ingres.gcf.util.SqlExFactory;


/*
** Name: SqlLongChar
**
** Description:
**	Class which represents an SQL LongVarchar value.  SQL 
**	LongVarchar values are potentially large variable length 
**	streams.
**
**	Supports conversion to String, Binary, Object.  
**
**	The data value accessor methods do not check for the NULL
**	condition.  The super-class isNull() method should first
**	be checked prior to calling any of the accessor methods.
**
**	This class supports both byte and character streams by
**	extending SqlLongNChar class and requiring a character-
**	set which defines the conversion between bytes strea and 
**	character stream and implementing the cnvtIS2Rdr method 
**	to wrap the InputStream with a Reader to perform the 
**	charactger-set conversion.
**
**  Inherited Methods:
**
**	setNull			Set a NULL data value.
**	toString		String representation of data value.
**	closeStream		Close the current stream.
**
**	setBoolean		Data value accessor assignment methods
**	setByte
**	setShort
**	setInt
**	setLong
**	setFloat
**	setDouble
**	setBigDecimal
**	setString
**	setDate
**	setTime
**	setTimestamp
**	setCharacterStream
**
**	getBoolean		Data value accessor retrieval methods
**	getByte
**	getShort
**	getInt
**	getLong
**	getFloat
**	getDouble
**	getBigDecimal
**	getString
**	getDate
**	getTime
**	getTimestamp
**	getAsciiStream
**	getUnicodeStream
**	getCharacterStream
**	getObject
**
**	strm2str		Convert stream to string.
**
**  Public Methods:
**
**	set			Assign a new non-NULL data value.
**	get			Retrieve current data value.
**
**	getBytes		Data value accessor retrieval methods
**	getBinaryStream
**
**  Protected Mehtods:
**
**	cnvtIS2Rdr		Convert InputStream to Reader.
**
**  Private Data:
**
**	charSet			Character-set.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	15-Nov-06 (gordy)
**	    Added strm2str() methods.
**	19-Dec-08 (gordy)
**	    Added getBytes(), getBinaryStream().
*/

public class
SqlLongChar
    extends SqlLongNChar
{

    private CharSet		charSet = null;

    
/*
** Name: SqlLongChar
**
** Description:
**	Class constructor.  Data value is initially NULL.
**
** Input:
**	charSet		Character-set of byte stream.
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
SqlLongChar( CharSet charSet )
{
    super();
    this.charSet = charSet;
} // SqlLongChar


/*
** Name: SqlLongChar
**
** Description:
**	Class constructor.  Data value is initially NULL.
**	Defines a SqlStream event listener for stream 
**	closure event notification.
**
** Input:
**	charSet		Character-set of byte stream.
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
SqlLongChar( CharSet charSet, StreamListener listener )
{
    super( listener );
    this.charSet = charSet;
} // SqlLongChar


/*
** Name: set
**
** Description:
**	Assign a new non-NULL data value.
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
**	Assign a new data value as a copy of a SqlChar data 
**	value.  The data value will be NULL if the input value 
**	is null, otherwise non-NULL.  
**
** Input:
**	data	SqlChar data value to copy.
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
set( SqlChar data )
    throws SQLException
{
    if ( data.isNull() )
	setNull();
    else
    {
	/*
	** The character data is stored in a byte array in
	** the host character-set.  A simple binary stream
	** will produce the desired output.  Note that we
	** need to follow the SqlChar convention and extend
	** the data to the optional limit.
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
**	Assign a new data value as a copy of a SqlVarChar data 
**	value.  The data value will be NULL if the input value 
**	is null, otherwise non-NULL.  
**
** Input:
**	data	SqlVarChar data value to copy.
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
set( SqlVarChar data )
    throws SQLException
{
    if ( data.isNull() )
	setNull();
    else
    {
	/*
	** The character data is stored in a byte array in
	** the host character-set.  A simple binary stream
	** will produce the desired output.
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
**	Reader	Current value.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public Reader
get()
    throws SQLException
{
    Object stream = getStream();
    Reader rdr;
    
    if ( stream == null )
	return( null );
    else  if ( stream instanceof InputStream )
	return( cnvtIS2Rdr( (InputStream)stream ) );
    else  if ( stream instanceof Reader )
	return( (Reader)stream );
    else
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
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
    Object stream = getStream();
    
    if ( stream != null )
	if ( stream instanceof InputStream )
	    copyIs2Os( (InputStream)stream, os );
	else  if ( stream instanceof Reader )
	{
	    Writer wtr;

	    try { wtr = charSet.getOSW( os ); }
	    catch( Exception ex )	// Should not happen!
	    { throw SqlExFactory.get( ERR_GC401E_CHAR_ENCODE ); }
	    
	    copyRdr2Wtr( (Reader)stream, wtr );
	}
	else
	    throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
    return;
} // get


/*
** Name: get
**
** Description:
**	Write the current data value to a Writer stream.
**	The current data value is consumed.  The Writer
**	stream is not closed but will be flushed.
**
**	Note: the value returned when the data value is 
**	NULL is not well defined.
**
** Input:
**	wtr	Writer to receive data value.
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
get( Writer wtr )
    throws SQLException
{
    Object stream = getStream();
    
    if ( stream != null )
	if ( stream instanceof InputStream )
	    copyRdr2Wtr( cnvtIS2Rdr( (InputStream)stream ), wtr );
	else  if ( stream instanceof Reader )
	    copyRdr2Wtr( (Reader)stream, wtr );
	else
	    throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
    return;
} // get


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
**	19-Dec-08 (gordy)
**	    Created.
*/

public byte[]
getBytes()
    throws SQLException
{
    Object stream = getStream();

    /*
    ** Returning the raw byte stream is only supported for non-
    ** Unicode data.
    */
    if ( stream == null )
	return( null );
    else  if ( stream instanceof InputStream )
	return( SqlLongByte.strm2array( (InputStream)stream ) );
    else
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
}


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
**	limit	Maximum size of result.
**
** Output:
**	None.
**
** Returns:
**	byte[]	Current value converted to byte array.
**
** History:
**	19-Dec-08 (gordy)
**	    Created.
*/

public byte[]
getBytes( int limit )
    throws SQLException
{
    Object stream = getStream();

    /*
    ** Returning the raw byte stream is only supported for non-
    ** Unicode data.
    */
    if ( stream == null )
	return( null );
    else  if ( stream instanceof InputStream )
	return( SqlLongByte.strm2array( (InputStream)stream, limit ) );
    else
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
}


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
** Name: cnvtIS2Rdr
**
** Description:
**	Converts the byte string input stream into a Reader
**	stream using the associated character-set.
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
    try { return( charSet.getISR( stream ) ); }
    catch( Exception ex )			// Should not happen!
    { throw SqlExFactory.get( ERR_GC401E_CHAR_ENCODE ); }
} // cnvtIS2Rdr


/*
** Name: strm2str
**
** Description:
**	Read a character input stream and convert to a string object.  
**	The stream is closed.
**
** Input:
**	in	Character input stream.
**	charSet	Character-set of input stream.
**
** Output:
**	None.
**
** Returns:
**	String	The stream as a string.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public static String
strm2str( InputStream in, CharSet cs )
    throws SQLException
{
    Reader rdr;

    /*
    ** Wrap stream in Reader capable of converting
    ** host character-set data into Java characters.
    */
    try { rdr = cs.getISR( in ); }
    catch( Exception ex )		// Should not happen!
    { throw SqlExFactory.get( ERR_GC401E_CHAR_ENCODE ); }

    /*
    ** Convert stream to string.
    */
    return( strm2str( rdr ) );
} // strm2str


/*
** Name: strm2str
**
** Description:
**	Read a character input stream and convert to a string object.  
**	The stream is closed.
**
** Input:
**	in	Character input stream.
**	charSet	Character-set of input stream.
**	limit	Max length of string.
**
** Output:
**	None.
**
** Returns:
**	String	The stream as a string.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public static String
strm2str( InputStream in, CharSet cs, int limit )
    throws SQLException
{
    Reader rdr;

    /*
    ** Wrap stream in Reader capable of converting
    ** host character-set data into Java characters.
    */
    try { rdr = cs.getISR( in ); }
    catch( Exception ex )		// Should not happen!
    { throw SqlExFactory.get( ERR_GC401E_CHAR_ENCODE ); }

    /*
    ** Convert stream to string.
    */
    return( strm2str( rdr, limit ) );
} // strm2str


} // class SqlLongChar
