/*
** Copyright (c) 2003, 2008 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.util;

/*
** Name: SqlVarChar.java
**
** Description:
**	Defines class which represents an SQL VarChar value.
**
**  Classes:
**
**	SqlVarChar	An SQL VarChar value.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Added support for parameter types/values in addition to 
**	    existing support for columns.
**	24-May-06 (gordy)
**	    Convert parsing exception to SQL conversion exception.
**	17-Nov-08 (rajus01) SIR 121238
**	    Replaced SqlEx references with SQLException as SqlEx becomes
**	    obsolete to support JDBC 4.0 SQLException hierarchy.
**	19-Dec-08 (gordy)
**	    Allow getBytes() and getBinaryStream().
**	24-Dec-08 (gordy)
**	    Use default date/time formatting instance.
*/

import	java.math.BigDecimal;
import	java.io.InputStream;
import	java.io.Reader;
import	java.io.UnsupportedEncodingException;
import	java.util.TimeZone;
import	java.sql.Date;
import	java.sql.Time;
import	java.sql.Timestamp;
import	java.sql.SQLException;


/*
** Name: SqlVarChar
**
** Description:
**	Class which represents an SQL VarChar value.  SQL VarChar 
**	values are variable length byte based strings.  A character-
**	set is associated with the byte string for conversion to a 
**	Java String.
**
**	Supports conversion to boolean, byte, short, int, long, float, 
**	double, BigDecimal, Date, Time, Timestamp, Object and streams.  
**
**	This class implements the ByteArray interface as the means
**	to set the string value.  The optional size limit defines
**	the maximum length of the string.  Capacity and length vary
**	as needed to handle the actual string size.
**
**	The string value may be set by first clearing the array and 
**	then using the put() method to set the value.  Segmented 
**	input values are handled by successive calls to put().  The 
**	clear() method also clears the NULL setting.
**
**	The data value accessor methods do not check for the NULL
**	condition.  The super-class isNull() method should first
**	be checked prior to calling any of the accessor methods.
**
**  Interface Methods:
**
**	capacity		Determine capacity of the array.
**	ensureCapacity		Set minimum capacity of the array.
**	limit			Set or determine maximum length of the array.
**	length			Determine the current length of the array.
**	clear			Set array length to zero.
**	set			Assign a new data value.
**	put			Copy bytes into the array.
**	get			Copy bytes out of the array.
**
**  Public Methods:
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
**	getBytes
**	getDate
**	getTime
**	getTimestamp
**	getBinaryStream
**	getAsciiStream
**	getUnicodeStream
**	getCharacterStream
**	getObject
**
**  Protected Data:
**
**	value			String value.
**	limit			Size limit.
**	length			Current length.
**	charSet			Character-set.
**
**  Protected Methods:
**
**	ensure			Ensure minimum capacity.
**
**  Private Data:
**
**	empty			Default empty string.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Removed dependency on SqlChar and implemented SqlData and
**	    ByteArray interfaces to support unlimited length strings.
**	    Added parameter data value oriented methods.
**	19-Dec-08 (gordy)
**	    Added getBytes(), getBinaryStream().
*/

public class
SqlVarChar
    extends SqlData
    implements ByteArray
{
    
    private static final byte	empty[] = new byte[ 0 ];

    protected byte		value[] = empty;
    protected int		limit = -1;	// Max (initially unlimited)
    protected int		length = 0;	// Current length
    protected CharSet		charSet = null;

    
/*
** Name: SqlVarChar
**
** Description:
**	Class constructor for variable length byte strings.  
**	Data value is initially NULL.
**
** Input:
**	charSet		Character-set of byte strings
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public
SqlVarChar( CharSet charSet )
{
    super( true );
    this.charSet = charSet;
} // SqlVarChar


/*
** Name: SqlVarChar
**
** Description:
**	Class constructor for limited length byte strings.  
**	Data value is initially NULL.
**
** Input:
**	charSet		Character-set of byte strings
**	size		The maximum string length.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted to new super class.
*/

public
SqlVarChar( CharSet charSet, int size )
{
    this( charSet );
    limit = size;
} // SqlVarChar


/*
** Name: capacity
**
** Description:
**	Returns the current capacity of the array.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Current capacity.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
*/

public int
capacity()
{
    return( value.length );
} // capacity


/*
** Name: ensureCapacity
**
** Description:
**	Set minimum capacity of the array.  Negative values,
**	values less than current capacity and greater than
**	(optional) limit are ignored.
**
** Input:
**	capacity	Minimum capacity.
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
**	    Adapted for SqlVarChar.
*/

public void
ensureCapacity( int capacity )
{
    ensure( capacity );
    return;
} // ensureCapacity


/*
** Name: limit
**
** Description:
**	Determine the current maximum size of the array.
**	If a maximum size has not been set, -1 is returned.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Maximum size or -1.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
*/

public int
limit()
{
    return( limit );
} // limit


/*
** Name: limit
**
** Description:
**	Set the maximum size of the array.  A negative size removes 
**	any prior size limit.  The array will be truncated if the 
**	current length is greater than the new maximum size.  Array 
**	capacity is not affected.
**
** Input:
**	size	Maximum size.
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
**	    Support unlimited length strings.
*/

public void
limit( int size )
{
    limit = (size < 0) ? -1 : size;
    if ( limit >= 0  &&  length > limit )  length = limit;
    return;
} // limit


/*
** Name: limit
**
** Description:
**	Set the maximum size of the array and optionally ensure
**	the mimimum capacity.  A negative size removes any prior 
**	size limit.  The array will be truncated if the current 
**	length is greater than the new maximum size.  Array
**	capacity will not decrease but may increase.
**
**	Note that limit(n, true) is equivilent to limit(n) followed 
**	by ensureCapacity(n) and limit(n, false) is the same as 
**	limit(n).
**
** Input:
**	size	Maximum size.
**	ensure	True to ensure capacity.
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
**	    Adapted for SqlVarChar.
*/

public void
limit( int size, boolean ensure )
{
    limit( size );
    if ( ensure )  ensure( size );
    return;
} // limit


/*
** Name: length
**
** Description:
**	Returns the current length of the array.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Current length.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
*/

public int
length()
{
    return( length );
} // length


/*
** Name: clear
**
** Description:
**	Sets the length of the array to zero.  Also clears NULL setting.
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
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
*/

public void
clear()
{
    setNotNull();
    length = 0;
    return;
} // clear


/*
** Name: set
**
** Description:
**	Assign a new data value as a copy of an existing 
**	SQL data object.  If the input is NULL, a NULL 
**	data value results.
**
** Input:
**	data	The SQL data to be copied.
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
{
    if ( data == null  ||  data.isNull() )
	setNull();
    else
    {
	clear();
	put( data.value, 0, data.length );
    }
    return;
} // set


/*
** Name: put
**
** Description:
**	Append a byte value to the current array data.  The portion
**	of the input which would cause the array to grow beyond the
**	(optional) size limit is ignored.  Number of bytes actually
**	appended is returned.
**
** Input:
**	value	The byte to append.
**
** Output:
**	None.
**
** Returns:
**	int	Number of bytes appended.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
*/

public int 
put( byte value )
{
    /*
    ** Is there room for the byte.
    */
    if ( limit >= 0  &&  length >= limit )  return( 0 );

    /*
    ** Save byte and update array length.
    */
    ensure( length + 1 );
    this.value[ length++ ] = value;
    return( 1 );
}


/*
** Name: put
**
** Description:
**	Append a byte array to the current array data.  The portion
**	of the input which would cause the array to grow beyond the
**	(optional) size limit is ignored.  The number of bytes actually 
**	appended is returned.
**
** Input:
**	value	    The byte array to append.
**
** Output:
**	None.
**
** Returns:
**	int	    Number of bytes appended.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
*/

public int
put( byte value[] )
{
    return( put( value, 0, value.length ) );
} // put


/*
** Name: put
**
** Description:
**	Append a portion of a byte array to the current array data.  
**	The portion of the input which would cause the array to grow 
**	beyond the (optional) size limit is ignored.  The number of 
**	bytes actually appended is returned.
**
** Input:
**	value	    Array containing bytes to be appended.
**	offset	    Start of portion to append.
**	length	    Number of bytes to append.
**
** Output:
**	None.
**
** Returns:
**	int	    Number of bytes appended.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
*/

public int
put( byte value[], int offset, int length )
{
    /*
    ** Determine how many bytes to copy.
    */
    int unused = (limit < 0) ? length : limit - this.length;
    if ( length > unused )  length = unused;
    
    /*
    ** Copy bytes and update array length.
    */
    ensure( this.length + length );
    System.arraycopy( value, offset, this.value, this.length, length );
    this.length += length;
    return( length );
} // put


/*
** Name: get
**
** Description:
**	Returns the byte value at a specific position in the
**	array.  If position is beyond the current array length, 
**	0 is returned.
**
** Input:
**	position	Array position.
**
** Output:
**	None.
**
** Returns:
**	byte		Byte value or 0.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public byte 
get( int position )
{
    return( (byte)((position >= length) ? 0 : value[ position ]) );
} // get


/*
** Name: get
**
** Description:
**	Returns a copy of the array.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	byte[]	Copy of the current array.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
*/

public byte[] 
get()
{
    byte bytes[] = new byte[ length ];
    System.arraycopy( value, 0, bytes, 0, length );
    return( bytes );
} // get


/*
** Name: get
**
** Description:
**	Copy bytes out of the array.  Copying starts with the first
**	byte of the array.  The number of bytes copied is the minimum
**	of the current array length and the length of the output array.
**
** Input:
**	None.
**
** Output:
**	value	Byte array to receive output.
**
** Returns:
**	int	Number of bytes copied.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
*/

public int
get( byte value[] )
{
    return( get( 0, value.length, value, 0 ) );
} // get


/*
** Name: get
**
** Description:
**	Copy a portion of the array.  Copying starts at the position
**	indicated.  The number of bytes copied is the minimum of the
**	length requested and the number of bytes in the array starting
**	at the requested position.  If position is greater than the
**	current length, no bytes are copied.  The output array must
**	have sufficient space.  The actual number of bytes copied is
**	returned.
**
** Input:
**	position	Starting byte to copy.
**	length		Number of bytes to copy.
**	offset		Starting position in output array.
**
** Output:
**	value		Byte array to recieve output.
**
** Returns:
**	int		Number of bytes copied.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
*/

public int
get( int position, int length, byte value[], int offset )
{
    /*
    ** Determine how many bytes to copy.
    */
    int avail = (position >= this.length) ? 0 : this.length - position;
    if ( length > avail )  length = avail;
    
    System.arraycopy( this.value, position, value, offset, length );
    return( length );
} // get


/*
** Name: ensure
**
** Description:
**	Set minimum capacity of the array.  Values less than
**	current capacity and greater than (optional) limit are
**	ignored.
**
** Input:
**	capacity	Required capacity.
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
**	13-Oct-03 (gordy)
**	    Forgot to reset value after copy into temp buffer.
**	 1-Dec-03 (gordy)
**	    Support unlimited string lengths.  Value never null.
*/

protected void
ensure( int capacity )
{
    /*
    ** Capacity does not need to exceed (optional) limit.
    */
    if ( limit >= 0  &&  capacity > limit )  capacity = limit;
    
    if ( capacity > value.length )
    {
	byte ba[] = new byte[ capacity ];
	System.arraycopy( value, 0, ba, 0, length );
	value = ba;
    }
    return;
} // ensure


/*
** Name: setBoolean
**
** Description:
**	Assign a boolean value as the non-NULL data value.
**
** Input:
**	value	Boolean value.
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
setBoolean( boolean value ) 
    throws SQLException
{
    setString( Boolean.toString( value ) );
    return;
}


/*
** Name: setByte
**
** Description:
**	Assign a byte value as the non-NULL data value.
**
** Input:
**	value	Byte value.
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
setByte( byte value ) 
    throws SQLException
{
    setString( Byte.toString( value ) );
    return;
}


/*
** Name: setShort
**
** Description:
**	Assign a short value as the non-NULL data value.
**
** Input:
**	value	Short value.
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
setShort( short value ) 
    throws SQLException
{
    setString( Short.toString( value ) );
    return;
}


/*
** Name: setInt
**
** Description:
**	Assign a integer value as the non-NULL data value.
**
** Input:
**	value	Integer value.
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
setInt( int value ) 
    throws SQLException
{
    setString( Integer.toString( value ) );
    return;
} // setInt


/*
** Name: setLong
**
** Description:
**	Assign a long value as the non-NULL data value.
**
** Input:
**	value	Long value.
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
setLong( long value ) 
    throws SQLException
{
    setString( Long.toString( value ) );
    return;
} // setLong


/*
** Name: setFloat
**
** Description:
**	Assign a float value as the non-NULL data value.
**
** Input:
**	value	Float value.
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
setFloat( float value ) 
    throws SQLException
{
    setString( Float.toString( value ) );
    return;
} // setFloat


/*
** Name: setDouble
**
** Description:
**	Assign a double value as the non-NULL data value.
**
** Input:
**	value	Double value.
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
setDouble( double value ) 
    throws SQLException
{
    setString( Double.toString( value ) );
    return;
} // setDouble


/*
** Name: setBigDecimal
**
** Description:
**	Assign a BigDecimal value as the data value.
**	The data value will be NULL if the input value
**	is null, otherwise non-NULL.
**
** Input:
**	value	BigDecimal value (may be null).
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
setBigDecimal( BigDecimal value ) 
    throws SQLException
{
    if ( value == null )
	setNull();
    else
	setString( value.toString() );

    return;
} // setBigDecimal


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
    {
	byte ba[];
	
	try { ba = charSet.getBytes( value ); }
	catch( UnsupportedEncodingException ex )
	{ throw SqlExFactory.get( ERR_GC401E_CHAR_ENCODE ); }
	
	/*
	** Normally, ensure() would be used to size the internal
	** array, possibly allocating a new array.  The problem
	** is that a new array is already going to be allocated 
	** for Unicode conversion.  Rather than use ensure, which
	** at best will do nothing and at worst will allocate an
	** additional array, we simply use the longer of internal
	** of conversion array.
	*/
	if ( ba.length > this.value.length )
	{
	    /*
	    ** First determine the length of the new string
	    ** (apply limit if applicable).
	    */
	    int length = ba.length;
	    if ( limit >= 0  &&  length > limit )  length = limit;
	    
	    /*
	    ** Now replace the internal buffer with the new string.
	    */
	    setNotNull();
	    this.value = ba;
	    this.length = length;
	}
	else
	{
	    /*
	    ** Since the converted string will fit in the current
	    ** internal buffer, we can just use standard access
	    ** methods to save the string.
	    */
	    clear();
	    put( ba );
	}
    }
    return;
} // setString


/*
** Name: setDate
**
** Description:
**	Assign a Date value as the data value.
**	The data value will be NULL if the input 
**	value is null, otherwise non-NULL.
**
**	A relative timezone may be provided which 
**	is applied to adjust the input such that 
**	it represents the original date/time value 
**	in the timezone provided.  The default is 
**	to use the local timezone.
**
** Input:
**	value	Date value (may be null).
**	tz	Relative timezone, NULL for local.
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
**	24-Dec-08 (gordy)
**	    Use default date/time formatter instance.
*/

public void 
setDate( Date value, TimeZone tz ) 
    throws SQLException
{
    if ( value == null )
	setNull();
    else  
    {
	/*
	** This conversion is not expected to be heavily used, 
	** therefore the default date/time formatters should 
	** be sufficient.
	*/
	SqlDates dates = SqlDates.getDefaultInstance();

	if ( tz != null )
	    setString( dates.formatDate( value, tz ) );
	else
	    setString( dates.formatDate( value, false ) );
    }
    return;
} // setDate


/*
** Name: setTime
**
** Description:
**	Assign a Time value as the data value.
**	The data value will be NULL if the input 
**	value is null, otherwise non-NULL.
**
**	A relative timezone may be provided which 
**	is applied to adjust the input such that 
**	it represents the original date/time value 
**	in the timezone provided.  The default is 
**	to use the local timezone.
**
** Input:
**	value	Time value (may be null).
**	tz	Relative timezone, NULL for local.
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
**	24-Dec-08 (gordy)
**	    Use default date/time formatter instance.
*/

public void 
setTime( Time value, TimeZone tz ) 
    throws SQLException
{
    if ( value == null )
	setNull();
    else  
    {
	/*
	** This conversion is not expected to be heavily used, 
	** therefore the default date/time formatters should 
	** be sufficient.
	*/
	SqlDates dates = SqlDates.getDefaultInstance();

	if ( tz != null )
	    setString( dates.formatTime( value, tz ) );
	else
	    setString( dates.formatTime( value, false ) );
    }
    return;
} // setTime


/*
** Name: setTimestamp
**
** Description:
**	Assign a Timestamp value as the data value.
**	The data value will be NULL if the input 
**	value is null, otherwise non-NULL.
**
**	A relative timezone may be provided which 
**	is applied to adjust the input such that 
**	it represents the original date/time value 
**	in the timezone provided.  The default is 
**	to use the local timezone.
**
** Input:
**	value	Timestamp value (may be null).
**	tz	Relative timezone, NULL for local.
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
**	24-Dec-08 (gordy)
**	    Use default date/time formatter instance.
*/

public void 
setTimestamp( Timestamp value, TimeZone tz ) 
    throws SQLException
{
    if ( value == null )
	setNull();
    else  
    {
	/*
	** This conversion is not expected to be heavily used, 
	** therefore the default date/time formatters should 
	** be sufficient.
	*/
	SqlDates dates = SqlDates.getDefaultInstance();

	if ( tz != null )
	    setString( dates.formatTimestamp( value, tz ) );
	else
	    setString( dates.formatTimestamp( value, false ) );
    }
    return;
} // setTimestamp


/*
** Name: getBoolean
**
** Description:
**	Return the current data value as a boolean value.
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
**	boolean		Current value converted to boolean.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
*/

public boolean 
getBoolean() 
    throws SQLException
{
    String str = getString( length ).trim();
    return( str.equals("1") ? true : Boolean.valueOf( str ).booleanValue());
} // getBoolean


/*
** Name: getByte
**
** Description:
**	Return the current data value as a byte value.
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
**	byte	Current value converted to byte.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
**	24-May-06 (gordy)
**	    Convert parsing exception to SQL conversion exception.
*/

public byte 
getByte() 
    throws SQLException
{
    byte value;
    
    try { value = Byte.parseByte( getString( length ).trim() ); }
    catch( NumberFormatException ex )
    { throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

    return( value );
} // getByte


/*
** Name: getShort
**
** Description:
**	Return the current data value as a short value.
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
**	short	Current value converted to short.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
**	24-May-06 (gordy)
**	    Convert parsing exception to SQL conversion exception.
*/

public short 
getShort() 
    throws SQLException
{
    short value;
    
    try { value = Short.parseShort( getString( length ).trim() ); }
    catch( NumberFormatException ex )
    { throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

    return( value );
} // getShort


/*
** Name: getInt
**
** Description:
**	Return the current data value as a integer value.
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
**	int	Current value converted to integer.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
**	24-May-06 (gordy)
**	    Convert parsing exception to SQL conversion exception.
*/

public int 
getInt() 
    throws SQLException
{
    int value;
    
    try { value = Integer.parseInt( getString( length ).trim() ); }
    catch( NumberFormatException ex )
    { throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

    return( value );
} // getInt


/*
** Name: getLong
**
** Description:
**	Return the current data value as a long value.
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
**	long	Current value converted to long.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
**	24-May-06 (gordy)
**	    Convert parsing exception to SQL conversion exception.
*/

public long 
getLong() 
    throws SQLException
{
    long value;
    
    try { value = Long.parseLong( getString( length ).trim() ); }
    catch( NumberFormatException ex )
    { throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

    return( value );
} // getLong


/*
** Name: getFloat
**
** Description:
**	Return the current data value as a float value.
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
**	float	Current value converted to float.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
**	24-May-06 (gordy)
**	    Convert parsing exception to SQL conversion exception.
*/

public float 
getFloat() 
    throws SQLException
{
    float value;
    
    try { value = Float.parseFloat( getString( length ).trim() ); }
    catch( NumberFormatException ex )
    { throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

    return( value );
} // getFloat


/*
** Name: getDouble
**
** Description:
**	Return the current data value as a double value.
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
**	double	Current value converted to double.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
**	24-May-06 (gordy)
**	    Convert parsing exception to SQL conversion exception.
*/

public double 
getDouble() 
    throws SQLException
{
    double value;
    
    try { value = Double.parseDouble( getString( length ).trim() ); }
    catch( NumberFormatException ex )
    { throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

    return( value );
} // getDouble


/*
** Name: getBigDecimal
**
** Description:
**	Return the current data value as a BigDecimal value.
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
**	BigDecimal	Current value converted to BigDecimal.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
**	24-May-06 (gordy)
**	    Convert parsing exception to SQL conversion exception.
*/

public BigDecimal 
getBigDecimal() 
    throws SQLException
{
    BigDecimal value;
    
    try { value = new BigDecimal( getString( length ).trim() ); }
    catch( NumberFormatException ex )
    { throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

    return( value );
} // getBigDecimal


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
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
*/

public String 
getString() 
    throws SQLException
{
    return( getString( length ) );
} // getString


/*
** Name: getString
**
** Description:
**	Return the current data value as a String value
**	with maximum size limit.
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
**	String	Current value converted to String.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
*/

public String 
getString( int limit ) 
    throws SQLException
{
    if ( limit > length )  limit = length;
    try { return( charSet.getString( value, 0, limit ) ); }
    catch( UnsupportedEncodingException ex )
    { throw SqlExFactory.get( ERR_GC401E_CHAR_ENCODE ); }
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
**	19-Dec-08 (gordy)
**	    Created.
*/

public byte[]
getBytes()
    throws SQLException
{
    return( getBytes( length ) );
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
    if ( limit > length )  limit = length;
    byte bytes[] = new byte[ limit ];
    get( 0, limit, bytes, 0 );
    return( bytes );
}


/*
** Name: getDate
**
** Description:
**	Return the current data value as a Date value.
**	A relative timezone may be provided which is
**	applied to adjust the final result such that it
**	represents the original date/time value in the
**	timezone provided.  The default is to use the
**	local timezone.
**
**	Note: the value returned when the data value is 
**	NULL is not well defined.
**
** Input:
**	tz	Relative timezone, NULL for local.
**	
**
** Output:
**	None.
**
** Returns:
**	Date	Current value converted to Date.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
**	24-Dec-08 (gordy)
**	    Use default date/time formatter instance.
*/

public Date 
getDate( TimeZone tz ) 
    throws SQLException
{ 
    /*
    ** This conversion is not expected to be heavily used, 
    ** therefore the default date/time formatters should 
    ** be sufficient.
    */
    SqlDates dates = SqlDates.getDefaultInstance();

    String str = getString( length ).trim();
    return( (tz == null) ? dates.parseDate( str, false )
			 : dates.parseDate( str, tz ) );
} // getDate


/*
** Name: getTime
**
** Description:
**	Return the current data value as a Time value.
**	A relative timezone may be provided which is
**	applied to adjust the final result such that it
**	represents the original date/time value in the
**	timezone provided.  The default is to use the
**	local timezone.
**
**	Note: the value returned when the data value is 
**	NULL is not well defined.
**
** Input:
**	tz	Relative timezone, NULL for local.
**
** Output:
**	None.
**
** Returns:
**	Time	Current value converted to Time.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
**	24-Dec-08 (gordy)
**	    Use default date/time formatter instance.
*/

public Time 
getTime( TimeZone tz ) 
    throws SQLException
{ 
    /*
    ** This conversion is not expected to be heavily used, 
    ** therefore the default date/time formatters should 
    ** be sufficient.
    */
    SqlDates dates = SqlDates.getDefaultInstance();

    String str = getString( length ).trim();
    return( (tz == null) ? dates.parseTime( str, false )
			 : dates.parseTime( str, tz ) );
} // getTime


/*
** Name: getTimestamp
**
** Description:
**	Return the current data value as a Timestamp value.
**	A relative timezone may be provided which is
**	applied to adjust the final result such that it
**	represents the original date/time value in the
**	timezone provided.  The default is to use the
**	local timezone.
**
**	Note: the value returned when the data value is 
**	NULL is not well defined.
**
** Input:
**	tz	Relative timezone, NULL for local.
**
** Output:
**	None.
**
** Returns:
**	Timestamp	Current value converted to Timestamp.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
**	24-Dec-08 (gordy)
**	    Use default date/time formatter instance.
*/

public Timestamp 
getTimestamp( TimeZone tz ) 
    throws SQLException
{ 
    /*
    ** This conversion is not expected to be heavily used, 
    ** therefore the default date/time formatters should 
    ** be sufficient.
    */
    SqlDates dates = SqlDates.getDefaultInstance();

    String str = getString( length ).trim();
    return( (tz == null) ? dates.parseTimestamp( str, false )
			 : dates.parseTimestamp( str, tz ) );
} // getTimestamp


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
**	19-Dec-08 (gordy)
**	    Created.
*/

public InputStream 
getBinaryStream() 
    throws SQLException
{ 
    return( getBinary( value, 0, length ) );
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
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
*/

public InputStream 
getAsciiStream() 
    throws SQLException
{ 
    return( getAscii( getString() ) );
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
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
*/

public InputStream 
getUnicodeStream() 
    throws SQLException
{ 
    return( getUnicode( getString() ) );
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
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
*/

public Reader 
getCharacterStream() 
    throws SQLException
{ 
    return( getCharacter( getString() ) );
} // getCharacterStream


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
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
*/

public Object 
getObject() 
    throws SQLException
{
    return( getString() );
} // getObject


/*
** Name: getObject
**
** Description:
**	Return the current data value as an Binary object
**	with maximum size limit.
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
**	Object	Current value converted to Binary.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Adapted for SqlVarChar.
*/

public Object 
getObject( int limit ) 
    throws SQLException
{
    return( getString( limit ) );
} // getObject


} // class SqlVarChar
