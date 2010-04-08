/*
** Copyright (c) 2007, 2008 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.util;

/*
** Name: SqlLongCharCache.java
**
** Description:
**	Defines class which represents an SQL Cached Long Varchar value.
**
**  Classes:
**
**	SqlLongCharCache	An SQL Cached Long Varchar value.
**
** History:
**	26-Feb-07 (gordy)
**	    Created.
**      17-Nov-08 (rajus01) SIR 121238
**          Replaced SqlEx references with SQLException or SqlExFactory
**          depending upon the usage of it. SqlEx becomes obsolete to support
**          JDBC 4.0 SQLException hierarchy.
**	19-Dec-08 (gordy)
**	    Allow getBytes().
*/

import	java.io.InputStream;
import	java.io.OutputStream;
import	java.io.Reader;
import	java.io.Writer;
import	java.io.UnsupportedEncodingException;
import	java.sql.SQLException;


/*
** Name: SqlLongCharCache
**
** Description:
**	Class which represents an SQL Cached Long Varchar value.  
**	SQL Long Varchar values are potentially large variable 
**	length streams.
**
**	Supports conversion to String, Binary, Object.  
**
**	The data value accessor methods do not check for the NULL
**	condition.  The super-class isNull() method should first
**	be checked prior to calling any of the accessor methods.
**
**	This class supports both byte and character streams by
**	extending SqlLongNChar class and requiring a character-
**	set which defines the conversion between byte stream and 
**	character stream.
**
**  Inherited Methods:
**
**	setNull			Set a NULL data value.
**	toString		String representation of data value.
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
**  Public Methods:
**
**	set			Assign a new non-NULL data value.
**	get			Retrieve current data value.
**
**	getBytes		Data value accessor retrieval methods.
**
**  Private Data:
**
**	charSet			Character-set.
**
**  Private Mthods:
**
**	cnvtIS2Rdr		Convert InputStream to Reader.
**	cnvtOS2Wtr		Convert OutputStream to Writer.
**
** History:
**	26-Feb-07 (gordy)
**	    Created.
**	19-Dec-08 (gordy)
**	    Added getBytes().
*/

public class
SqlLongCharCache
    extends SqlLongNCharCache
{

    private CharSet		charSet = null;

    
/*
** Name: SqlLongCharCache
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public
SqlLongCharCache( CharSet charSet )
{
    super();
    this.charSet = charSet;
} // SqlLongCharCache


/*
** Name: SqlLongCharCache
**
** Description:
**	Class constructor.  Data value is initially NULL.
**
** Input:
**	charSet		Character-set of byte stream.
**	segSize		Segment size.
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
SqlLongCharCache( CharSet charSet, int segSize )
{
    super( segSize );
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
	set( cnvtIS2Rdr( stream ) );
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public void
set( SqlChar data )
    throws SQLException
{
    if ( data == null  ||  data.isNull() )
	setNull();
    else
    {
	/*
	** The character data is stored in a byte array in
	** the host character-set.  Note that we need to 
	** follow the SqlChar convention and extend the data 
	** to the optional limit.
	*/
	data.extend();
	set( cnvtIS2Rdr( getBinary( data.value, 0, data.length ) ) );
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public void
set( SqlVarChar data )
    throws SQLException
{
    if ( data == null  ||  data.isNull() )
	setNull();
    else
    {
	/*
	** The character data is stored in a byte array in
	** the host character-set.
	*/
	set( cnvtIS2Rdr( getBinary( data.value, 0, data.length ) ) );
    }
    return;
} // set


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
    get( cnvtOS2Wtr( os ) );
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
    byte ba[];

    /*
    ** String is cached as characters, but the easiest access
    ** is by the superclass getString() method.  The characters
    ** are then obtained from the string and converted to bytes
    ** using the associated CharSet.
    */
    try { ba = charSet.getBytes( getString().toCharArray() ); }
    catch( UnsupportedEncodingException ex )
    { throw SqlExFactory.get( ERR_GC401E_CHAR_ENCODE ); }

    return( ba );
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
    byte ba[];

    /*
    ** String is cached as characters, but the easiest access
    ** is by the superclass getString() method.  The characters
    ** are then obtained from the string and converted to bytes
    ** using the associated CharSet.
    */
    try { ba = charSet.getBytes( getString( limit ).toCharArray() ); }
    catch( UnsupportedEncodingException ex )
    { throw SqlExFactory.get( ERR_GC401E_CHAR_ENCODE ); }

    return( ba );
}


/*
** Name: cnvtIS2Rdr
**
** Description:
**	Converts the byte input stream into a character Reader
**	stream using the associated character-set.
**
** Input:
**	strea	Byte input stream.
**
** Output:
**	None.
**
** Returns:
**	Reader	Character Reader stream.
**
** History:
**	26-Feb-07 (gordy)
*/

private Reader
cnvtIS2Rdr( InputStream stream )
    throws SQLException
{
    try { return( charSet.getISR( stream ) ); }
    catch( Exception ex )			// Should not happen!
    { throw SqlExFactory.get( ERR_GC401E_CHAR_ENCODE ); }
} // cnvtIS2Rdr


/*
** Name: cnvtOS2Wtr
**
** Description:
**	Converts the byte output stream into a character Writer
**	stream using the associated character-set.
**
** Input:
**	stream	Byte output stream.
**
** Output:
**	None.
**
** Returns:
**	Reader	Character Writer stream.
**
** History:
**	26-Feb-07 (gordy)
*/

private Writer
cnvtOS2Wtr( OutputStream stream )
    throws SQLException
{
    try { return( charSet.getOSW( stream ) ); }
    catch( Exception ex )			// Should not happen!
    { throw SqlExFactory.get( ERR_GC401E_CHAR_ENCODE ); }
} // cnvtOS2Wtr


} // class SqlLongCharCache
