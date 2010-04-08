/*
** Copyright (c) 2007, 2009 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.util;

/*
** Name: SqlLongNCharCache.java
**
** Description:
**	Defines class which represents an SQL Cached Long NVarchar value.
**
**  Classes:
**
**	SqlLongNCharCache	An SQL Cached Long NVarchar value.
**
** History:
**	26-Feb-07 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Use default date/time formatting instance.
*/

import	java.math.BigDecimal;
import	java.io.InputStream;
import	java.io.Reader;
import	java.io.Writer;
import	java.io.IOException;
import	java.util.TimeZone;
import	java.sql.Date;
import	java.sql.Time;
import	java.sql.Timestamp;
import	java.sql.Clob;
import	java.sql.NClob;
import  java.sql.SQLException;


/*
** Name: SqlLongNCharCache
**
** Description:
**	Class which represents an SQL Cached Long NVarchar value.  
**	SQL Long NVarchar values are potentially large variable 
**	length streams.
**
**	Supports conversion to String, Binary, Object.  
**
**	The data value accessor methods do not check for the NULL
**	condition.  The super-class isNull() method should first
**	be checked prior to calling any of the accessor methods.
**
**  Public Methods:
**
**	set			Assign a new non-NULL data value.
**	get			Retrieve current data value.
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
**	getClob
**	getNlob
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
**      17-Nov-08 (rajus01) SIR 121238
**          Replaced SqlEx references with SQLException or SqlExFactory
**          depending upon the usage of it. SqlEx becomes obsolete to support
**          JDBC 4.0 SQLException hierarchy.
*/

public class
SqlLongNCharCache
    extends SqlData
{

    private CharBuffer		buffer = null;
    private int			segSize = 8192;


/*
** Name: SqlLongNCharCache
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
SqlLongNCharCache()
{
    super( true );
} // SqlLongNCharCache


/*
** Name: SqlLongNCharCache
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
SqlLongNCharCache( int segSize )
{
    super( true );
    this.segSize = segSize;
} // SqlLongNCharCache


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
set( Reader stream )
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
	buffer = new CharBuffer( segSize, stream );
    }
    return;
} // set


/*
** Name: set
**
** Description:
**	Assign a new data value as a copy of a SqlNChar data 
**	value.  The data value will be NULL if the input value 
**	is null, otherwise non-NULL.  
**
** Input:
**	data	SqlNChar data value to copy.
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
set( SqlNChar data )
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
	** The character data is stored in a character array.  
	** Note that we need to follow the SqlNChar convention 
	** and extend the data to the optional limit.
	*/
	setNotNull();
	data.extend();
	buffer = new CharBuffer( segSize, data.value, 0, data.length );
    }
    return;
} // set


/*
** Name: set
**
** Description:
**	Assign a new data value as a copy of a SqlNVarChar data 
**	value.  The data value will be NULL if the input value 
**	is null, otherwise non-NULL.  
**
** Input:
**	data	SqlNVarChar data value to copy.
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
set( SqlNVarChar data )
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
	** The character data is stored in a character array.  
	*/
	setNotNull();
	buffer = new CharBuffer( segSize, data.value, 0, data.length );
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public Reader
get()
    throws SQLException
{
    return( buffer.getRdr() );
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public void
get( Writer wtr )
    throws SQLException
{
    buffer.read( wtr );
    return;
} // get


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
**	26-Feb-07 (gordy)
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
**	26-Feb-07 (gordy)
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
**	26-Feb-07 (gordy)
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
**	26-Feb-07 (gordy)
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
**	26-Feb-07 (gordy)
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
**	26-Feb-07 (gordy)
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
**	26-Feb-07 (gordy)
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
**	26-Feb-07 (gordy)
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
    {
	/*
	** A new buffer is created for each new value because
	** the previous value may be referenced from an external
	** object returned by one of the accessor methods.
	*/
	setNotNull();
	buffer = new CharBuffer( segSize, value, 0, value.length() );
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
**	26-Feb-07 (gordy)
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
**	26-Feb-07 (gordy)
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
**	26-Feb-07 (gordy)
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
** Name: setAsciiStream
**
** Description:
**	Assign a ASCII stream as the data value.
**	The data value will be NULL if the input 
**	value is null, otherwise non-NULL.
**
** Input:
**	value	ASCII stream (may be null).
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
setAsciiStream( InputStream value ) 
    throws SQLException
{
    if ( value == null )
	setNull();
    else
	setCharacterStream( cnvtAscii( value ) );
    return;
} // setAsciiStream


/*
** Name: setUnicodeStream
**
** Description:
**	Assign a Unicode stream as the data value.
**	The data value will be NULL if the input 
**	value is null, otherwise non-NULL.
**
** Input:
**	value	Unicode stream (may be null).
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
setUnicodeStream( InputStream value ) 
    throws SQLException
{
    if ( value == null )
	setNull();
    else
	setCharacterStream( cnvtUnicode( value ) );
    return;
} // setAsciiStream


/*
** Name: setCharacterStream
**
** Description:
**	Assign a character stream as the data value.
**	The data value will be NULL if the input 
**	value is null, otherwise non-NULL.
**
** Input:
**	value	Character stream (may be null).
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
setCharacterStream( Reader value ) 
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
	buffer = new CharBuffer( segSize, value );
    }
    return;
} // setCharacterStream


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
**	26-Feb-07 (gordy)
**	    Created.
*/

public boolean 
getBoolean() 
    throws SQLException
{
    String str = getString().trim();
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public byte 
getByte() 
    throws SQLException
{
    byte value;
    
    try { value = Byte.parseByte( getString().trim() ); }
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public short 
getShort() 
    throws SQLException
{
    short value;
    
    try { value = Short.parseShort( getString().trim() ); }
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public int 
getInt() 
    throws SQLException
{
    int value;
    
    try { value = Integer.parseInt( getString().trim() ); }
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public long 
getLong() 
    throws SQLException
{
    long value;
    
    try { value = Long.parseLong( getString().trim() ); }
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public float 
getFloat() 
    throws SQLException
{
    float value;
    
    try { value = Float.parseFloat( getString().trim() ); }
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public double 
getDouble() 
    throws SQLException
{
    double value;
    
    try { value = Double.parseDouble( getString().trim() ); }
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
**	26-Feb-07 (gordy)
**	    Created.
*/

public BigDecimal 
getBigDecimal() 
    throws SQLException
{
    BigDecimal value;
    
    try { value = new BigDecimal( getString().trim() ); }
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
**	String		Current value converted to String.
**
** History:
**	26-Feb-07 (gordy)
**	    Created.
*/

public String 
getString() 
    throws SQLException
{
    /*
    ** There is a natural limit at the max integer length.
    */
    return( getString( Integer.MAX_VALUE ) );
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
    limit = (int)Math.min( (long)limit, buffer.length() );
    char chars[] = new char[ limit ];
    buffer.read( 0, chars, 0, limit );
    return( new String( chars ) );
} // getString


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
**	26-Feb-07 (gordy)
**	    Created.
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

    String str = getString().trim();
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
**	26-Feb-07 (gordy)
**	    Created.
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

    String str = getString().trim();
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
**	26-Feb-07 (gordy)
**	    Created.
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

    String str = getString().trim();
    return( (tz == null) ? dates.parseTimestamp( str, false )
			 : dates.parseTimestamp( str, tz ) );
}


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
    return( SqlStream.getAsciiIS( buffer.getRdr() ) );
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
    return( SqlStream.getUnicodeIS( buffer.getRdr() ) );
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
    return( buffer.getRdr() );
} // getCharacterStream


/*
** Name: getClob
**
** Description:
**	Returns the current data value buffered in a Clob wrapper.
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
**      Clob    Current value buffered and wrapped as a Clob.
**
** History:
**      26-Feb-07 (gordy)
**          Created.
*/

public Clob
getClob()
    throws SQLException
{
    return( new BufferedClob( buffer ) );
}

/*
** Name: getNlob
**
** Description:
**	Returns the current data value buffered in a NClob wrapper.
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
**      NClob    Current value buffered and wrapped as a NClob.
**
** History:
**      9-Apr-09 (rajus01) SIR 121238
**          Created.
**	 5-Jun-09 (gordy)
**	    Fixed class reference.
*/
public NClob
getNlob()
    throws SQLException
{
    return( new BufferedNlob( buffer ) );
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
    return( getString( limit ) );
} // getObject


} // class SqlLongNCharCache
