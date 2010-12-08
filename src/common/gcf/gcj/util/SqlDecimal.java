/*
** Copyright (c) 2003, 2008 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.util;

/*
** Name: SqlDecimal.java
**
** Description:
**	Defines class which represents an SQL Decimal value.
**
**  Classes:
**
**	SqlDecimal	An SQL Decimal value.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Added support for parameter types/values in addition to 
**	    existing support for columns.
**	 4-Aug-05 (gordy)
**	    Enforce the Ingres max decimal precision of 31 by rounding
**	    scale digits to reduce the precision.
**	 7-Jun-07 (gordy)  BUG 118470
**	    BigDecimal.toString() output changed with Java 1.5.
**	 9-Sep-08 (gordy)
**	    Allow max precision to be specified.
**      17-Nov-08 (rajus01) SIR 121238
**          Replaced SqlEx references with SQLException or SqlExFactory
**          depending upon the usage of it. SqlEx becomes obsolete to support
**          JDBC 4.0 SQLException hierarchy.
*/

import	java.math.BigDecimal;
import	java.sql.SQLException;


/*
** Name: SqlDecimal
**
** Description:
**	Class which represents an SQL Decimal value.  
**
**	Supports conversion to boolean, byte, short, integer, long, 
**	float, double, String and Object.
**
**	The data value accessor methods do not check for the NULL
**	condition.  The super-class isNull() method should first
**	be checked prior to calling any of the accessor methods.
**
**	Decimal values are truncatable.  A truncated decimal value
**	results in data value of 0.0.
**
**  Constants
**	PRECISION_MAX_DEFAULT	Default maximum precision.
**
**  Public Methods:
**
**	set			Assign a new data value.
**	get			Retrieve current data value.
**	isTruncated		Data value is truncated?
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
**	getObject
**
**  Protected Methods:
**
**	setScale		Set decimal scale
**
**  Private Data:
**
**	value			The decimal value.
**	truncated		Decimal value is truncted.
**	maxPrecision		Maximum precision.
**
**  Private Methods:
**	checkPrecision		Limit precision to max.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Added parameter data value oriented methods.
**	 4-Aug-05 (gordy)
**	    Added maxPrecision constant.
**	 9-Sep-08 (gordy)
**	    Max precision may vary.  Default max precision provided
**	    as public constant.  Added construct which allows max 
**	    precision to be specified.
**	 9-Nov-10 (gordy & rajus01) Bug 124712
**	    Re-worked conversion of BigDecimal precision and scale into
**	    the appropriate Ingres precision and scale. The cases when
**	    a precision being smaller than scale or scale could be 
**	    negative are handled correcty when converting to Ingres decimal
**	    format. Some examples of BigDecimal precision and scale values:
**	    "20000000"  => precision:8 scale: 0
**	    "2E7"	=> precision:1 scale: -7
**	    "2.0E7"	=> precision:2 scale: -6
*/

public class
SqlDecimal
    extends SqlData
{

    public static final int	PRECISION_MAX_DEFAULT = 31;

    private BigDecimal		value = null;
    private boolean		truncated = false;
    private int			maxPrecision = PRECISION_MAX_DEFAULT;


/*
** Name: SqlDecimal
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
SqlDecimal()
{
    super( true );
} // SqlDecimal


/*
** Name: SqlDecimal
**
** Description:
**	Class constructor.  Allows specification of max precision.
**	Data value is initially NULL.
**
** Input:
**	maxPrecision	Maximum precision.
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
SqlDecimal( int maxPrecision )
{
    this();
    this.maxPrecision = maxPrecision;
} // SqlDecimal


/*
** Name: set
**
** Description:
**	Assign a new data value.  If the input is NULL,
**	a NULL data value results.
**
** Input:
**	value		The new data value.
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
*/

public void
set( String value )
{
    if ( value == null )
    {
	setNull();
	truncated = false;
    }
    else  if ( value.length() > 0 )
    {
	this.value = new BigDecimal( value );
	setNotNull();
	truncated = false;
    }
    else
    {
	this.value = new BigDecimal( 0.0 );
	setNotNull();
	truncated = true;
    }
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
set( SqlDecimal data )
{
    if ( data == null  ||  data.isNull() )
    {
	setNull();
	truncated = false;
    }
    else
    {
	setNotNull();
	value = data.value;
	truncated = data.truncated;
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
**	 7-Jun-07 (gordy)
**	    BigDecimal.toString() output changed with Java 1.5.  Use
**	    new toPlainString() method which produces original format.
*/

public String 
get() 
{
    return( value.toPlainString() );
} // get


/*
** Name: isTruncated
**
** Description:
**	Returns an indication that the data value was truncated.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean		True if truncated.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
*/

public boolean
isTruncated()
{
    return( truncated );
} // isTruncated


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
    setNotNull();
    truncated = false;
    this.value = BigDecimal.valueOf( value ? 1L : 0L );
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
    setNotNull();
    truncated = false;
    this.value = BigDecimal.valueOf( (long)value );
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
    setNotNull();
    truncated = false;
    this.value = BigDecimal.valueOf( (long)value );
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
    setNotNull();
    truncated = false;
    this.value = BigDecimal.valueOf( (long)value );
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
    setNotNull();
    truncated = false;
    this.value = BigDecimal.valueOf( value );
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
**	 4-Aug-05 (gordy)
**	    Adjust precision to valid range.
*/

public void 
setFloat( float value ) 
    throws SQLException
{
    BigDecimal temp;

    try { temp = new BigDecimal( (double)value ); }
    catch( NumberFormatException ex )
    { throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

    this.value = checkPrecision( temp );
    setNotNull();
    truncated = false;
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
**	 4-Aug-05 (gordy)
**	    Adjust precision to valid range.
*/

public void 
setDouble( double value ) 
    throws SQLException
{
    BigDecimal temp;

    try { temp = new BigDecimal( value ); }
    catch( NumberFormatException ex )
    { throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

    this.value = checkPrecision( temp );
    setNotNull();
    truncated = false;
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
**	 4-Aug-05 (gordy)
**	    Adjust precision to valid range.
*/

public void 
setBigDecimal( BigDecimal value ) 
    throws SQLException
{
    if ( value == null )
	setNull();
    else
    {
	this.value = checkPrecision( value );
	setNotNull();
    }
    
    truncated = false;
    return;
} // setBigDecimal


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
**	scale	Number of decimal places.
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
**	 4-Aug-05 (gordy)
**	    Adjust precision to valid range.
*/

public void 
setBigDecimal( BigDecimal value, int scale ) 
    throws SQLException
{
    if ( value == null )
	setNull();
    else
    {
	value = value.setScale( scale, BigDecimal.ROUND_HALF_UP );
	this.value = checkPrecision( value );
	setNotNull();
    }
    
    truncated = false;
    return;
} // setBigDecimal


/*
** Name: setScale
**
** Description:
**	Set the scale of the DECIMAL value.
**
** Input:
**	scale	Number of decimal places.
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
**	 4-Aug-05 (gordy)
**	    Adjust precision to valid range.
*/

protected void
setScale( int scale )
    throws SQLException
{
    if ( value != null )
	if ( value.scale() < scale )	/* Raising scale raises precision */
	    value = checkPrecision( value.setScale(scale, 
						   BigDecimal.ROUND_HALF_UP) );
	else
	    value = value.setScale( scale, BigDecimal.ROUND_HALF_UP );
    return;
} // setScale


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
**	 4-Aug-05 (gordy)
**	    Adjust precision to valid range.
*/

public void 
setString( String value ) 
    throws SQLException
{
    if ( value == null )
	setNull();
    else
    {
	BigDecimal temp;

	try { temp = new BigDecimal( value.trim() ); }
	catch( NumberFormatException ex )
	{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }
	
	this.value = checkPrecision( temp );
	setNotNull();
    }
    
    truncated = false;
    return;
} // setString


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
    return( (value.signum() == 0) ? false : true );
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
*/

public byte 
getByte() 
    throws SQLException
{
    return( value.byteValue() );
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
*/

public short 
getShort() 
    throws SQLException
{
    return( value.shortValue() );
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
*/

public int 
getInt() 
    throws SQLException
{
    return( value.intValue() );
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
*/

public long 
getLong() 
    throws SQLException
{
    return( value.longValue() );
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
*/

public float 
getFloat() 
    throws SQLException
{
    return( value.floatValue() );
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
*/

public double 
getDouble() 
    throws SQLException
{
    return( value.doubleValue() );
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
*/

public BigDecimal 
getBigDecimal() 
    throws SQLException
{
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
**	 5-Sep-03 (gordy)
**	    Created.
**	 7-Jun-07 (gordy)
**	    BigDecimal.toString() output changed with Java 1.5.  Use
**	    new toPlainString() method which produces original format.
*/

public String 
getString() 
    throws SQLException
{
    return( value.toPlainString() );
} // getString


/*
** Name: getObject
**
** Description:
**	Return the current data value as a BigDecimal object.
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
**	Object	Current value converted to BigDecimal.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
*/

public Object 
getObject() 
    throws SQLException
{
    return( getBigDecimal() );
} // getObject


/*
** Name: checkPrecision
**
** Description:
**	Verifies that precision of a BigDecimal value is within
**	the range supported by the Ingres decimal data-type.
**	Returns original value if precision is OK.  Returns an 
**	updated value if precision exceeds range and value can 
**	be adjusted (scaled) with minimal change, otherwise an
**	exception is thrown.
**
** Input:
**	value		BigDecimal to check.
**
** Output:
**	None.
**
** Returns:
**	BigDecimal	Original or updated value.
**
** History:
**	 4-Aug-05 (gordy)
**	    Created.
**	 9-Sep-08 (gordy)
**	    maxPrecision is no longer static, so method cannot be static.
*/

private BigDecimal
checkPrecision( BigDecimal value )
    throws SQLException
{
    /*
    ** In BigDecimal, precision is the number of digits in
    ** the numeric portion of the value, excluding leading
    ** 0's (even following the decimal point).  Scale is a
    ** combination of the position of the decimal point and
    ** any exponent.
    */
    int precision = value.precision();
    int	scale = value.scale();

    /*
    ** In Ingres decimals, precision is the total number of
    ** possible digits or size of the value and only
    ** excludes leading 0's preceding the decimal point.
    ** Scale is simply the position of the decimal point.
    **
    ** If the BigDecimal scale is negative, then the decimal
    ** point must be moved to the right with a corresponding
    ** increase in precision.
    */
    if ( scale < 0 )
    {
 	precision += -scale;
	scale = 0;
    } 

    /*
    ** Precision must be large enough to hold scale portion.
    */
    if ( precision < scale )  precision = scale;
 
    /*
    ** BigDecimal can represent values larger than can be held
    ** in an Ingres decimal.  It may be possible to truncate
    ** the value to fit within the maximum precision.
    */

    if ( precision > maxPrecision )
    {
	/*
	** Precision can be adjusted by truncating decimal
	** digits (reducing scale by rounding).  Conversion
	** exception is thrown if insufficient scale is
	** available (can't change number of integer digits).
	*/
	int digits = precision - maxPrecision;

	if ( scale >= digits )
	    // TODO: set truncation indication
	    value = value.setScale( scale - digits, BigDecimal.ROUND_HALF_UP );
	else
	    throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
    }

    return( value );
} // checkPrecision


} // class SqlDecimal
