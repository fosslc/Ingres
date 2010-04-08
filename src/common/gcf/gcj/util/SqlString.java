/*
** Copyright (c) 2003, 2008 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.util;

/*
** Name: SqlString.java
**
** Description:
**	Defines class which represents an SQL string value.
**
**  Classes:
**
**	SqlChar		An SQL Char value.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Added support for parameter types/values in addition to 
**	    existing support for columns.
**	24-May-06 (gordy)
**	    Convert parsing exception to SQL conversion exception.
**      17-Nov-08 (rajus01) SIR 121238
**          Replaced SqlEx references with SQLException as SqlEx becomes
**          obsolete to support JDBC 4.0 SQLException hierarchy.
**	19-Dec-08 (gordy)
**	    Allow getBytes() and getBinaryStream().
**	24-Dec-08 (gordy)
**	    Use default date/time formatting instance.
*/

import	java.math.BigDecimal;
import	java.io.InputStream;
import	java.io.Reader;
import	java.util.TimeZone;
import	java.sql.Date;
import	java.sql.Time;
import	java.sql.Timestamp;
import	java.sql.SQLException;


/*
** Name: SqlString
**
** Description:
**	Class which represents an SQL string value.  SQL string 
**	values are represented by Java Strings
**
**	Supports conversion to boolean, byte, short, int, long, float, 
**	double, BigDecimal, Date, Time, Timestamp, Object and streams.  
**
**	The data value accessor methods do not check for the NULL
**	condition.  The super-class isNull() method should first
**	be checked prior to calling any of the accessor methods.
**
**  Public Methods:
**
**	set			Assign a new data value.
**	get			Retrieve current data value.
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
**  Private Data:
**
**	value			String value.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Added parameter data value oriented methods.  Default
**	    functionality moved to base class.
**	19-Dec-08 (gordy)
**	    Added getBytes(), getBinaryStream().
*/

public class
SqlString
    extends SqlData
{

    private String		value = null;

    
/*
** Name: SqlString
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
**	 5-Sep-03 (gordy)
**	    Created.
*/

public
SqlString()
{
    super( true );
} // SqlString


/*
** Name: SqlString
**
** Description:
**	Class constructor for non-NULL data values.
**
** Input:
**	value	Initial string value.
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
*/

public
SqlString( String value )
{
    super( false );
    this.value = value;
} // SqlString


/*
** Name: Set
**
** Description:
**	Assign a new non-NULL data value.
**
** Input:
**	value	New string value.
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
*/

public void
set( String value )
{
    setNotNull();
    this.value = value;
    return;
} // set


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
set( SqlString data )
{
    if ( data == null  ||  data.isNull() )
	setNull();
    else
    {
	setNotNull();
	value = data.value;
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
**	String	Current value.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public String 
get() 
{
    return( value );
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
	setNotNull();
	this.value = value;
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
*/

public boolean 
getBoolean() 
    throws SQLException
{
    String str = value.trim();
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
**	24-May-06 (gordy)
**	    Convert parsing exception to SQL conversion exception.
*/

public byte 
getByte() 
    throws SQLException
{
    byte val;
    
    try { val = Byte.parseByte( value.trim() ); }
    catch( NumberFormatException ex )
    { throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

    return( val );
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
**	24-May-06 (gordy)
**	    Convert parsing exception to SQL conversion exception.
*/

public short 
getShort() 
    throws SQLException
{
    short val;
    
    try { val = Short.parseShort( value.trim() ); }
    catch( NumberFormatException ex )
    { throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

    return( val );
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
**	24-May-06 (gordy)
**	    Convert parsing exception to SQL conversion exception.
*/

public int 
getInt() 
    throws SQLException
{
    int val;
    
    try { val = Integer.parseInt( value.trim() ); }
    catch( NumberFormatException ex )
    { throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

    return( val );
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
**	24-May-06 (gordy)
**	    Convert parsing exception to SQL conversion exception.
*/

public long 
getLong() 
    throws SQLException
{
    long val;
    
    try { val = Long.parseLong( value.trim() ); }
    catch( NumberFormatException ex )
    { throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

    return( val );
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
**	24-May-06 (gordy)
**	    Convert parsing exception to SQL conversion exception.
*/

public float 
getFloat() 
    throws SQLException
{
    float val;
    
    try { val = Float.parseFloat( value.trim() ); }
    catch( NumberFormatException ex )
    { throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

    return( val );
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
**	24-May-06 (gordy)
**	    Convert parsing exception to SQL conversion exception.
*/

public double 
getDouble() 
    throws SQLException
{
    double val;
    
    try { val = Double.parseDouble( value.trim() ); }
    catch( NumberFormatException ex )
    { throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

    return( val );
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
**	24-May-06 (gordy)
**	    Convert parsing exception to SQL conversion exception.
*/

public BigDecimal 
getBigDecimal() 
    throws SQLException
{
    BigDecimal val;
    
    try { val = new BigDecimal( value.trim() ); }
    catch( NumberFormatException ex )
    { throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

    return( val );
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
*/

public String 
getString() 
    throws SQLException
{
    return( value );
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
*/

public String 
getString( int limit ) 
    throws SQLException
{
    return( (limit < value.length()) ? value.substring( 0, limit ) : value );
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
    /*
    ** Since Java strings are being represented, there isn't
    ** a reference charset, so the platform default is used.
    */
    return( value.getBytes() );
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
    String val = (limit < value.length()) ? value.substring(0, limit) : value;
    return( val.getBytes() );
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

    return( (tz == null) ? dates.parseDate( value.trim(), false )
			 : dates.parseDate( value.trim(), tz ) );
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

    return( (tz == null) ? dates.parseTime( value.trim(), false )
			 : dates.parseTime( value.trim(), tz ) );
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

    return( (tz == null) ? dates.parseTimestamp( value.trim(), false )
			 : dates.parseTimestamp( value.trim(), tz ) );
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
**	19-Dec-08 (gordy)
**	    Created.
*/

public InputStream 
getBinaryStream() 
    throws SQLException
{ 
    /*
    ** Since Java strings are being represented, there isn't
    ** a reference charset, so the platform default is used.
    */
    byte val[] = value.getBytes();
    return( getBinary( val, 0, val.length ) );
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
**	    Utilize helper function in base class.
*/

public InputStream 
getAsciiStream() 
    throws SQLException
{ 
    return( getAscii( value ) );
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
**	    Utilize helper function in base class.
*/

public InputStream 
getUnicodeStream() 
    throws SQLException
{
    return( getUnicode( value ) );
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
**	    Utilize helper function in base class.
*/

public Reader 
getCharacterStream() 
    throws SQLException
{ 
    return( getCharacter( value ) );
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
*/

public Object 
getObject( int limit ) 
    throws SQLException
{
    return( getString( limit ) );
} // getObject


} // class SqlString
