/*
** Copyright (c) 2003, 2009 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.util;

/*
** Name: SqlBigInt.java
**
** Description:
**	Defines class which represents an SQL Bigint value.
**
**  Classes:
**
**	SqlBigInt	An SQL Bigint value.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Added support for parameter types/values in addition to 
**	    existing support for columns.
**      17-Nov-08 (rajus01) SIR 121238
**          Replaced SqlEx references with SQLException or SqlExFactory
**          depending upon the usage of it. SqlEx becomes obsolete to support
**          JDBC 4.0 SQLException hierarchy.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
*/

import	java.math.BigDecimal;
import	java.sql.SQLException;


/*
** Name: SqlBigInt
**
** Description:
**	Class which represents an SQL Bigint value.  
**
**	Supports conversion to boolean, byte, short, integer, float, 
**	double, BigDecimal, String and Object.
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
**  Private Data:
**
**	value			The long value.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Added parameter data value oriented methods.
*/

public class
SqlBigInt
    extends SqlData
{

    private long		value = 0;


/*
** Name: SqlBigInt
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
SqlBigInt()
{
    super( true );
} // SqlBigInt


/*
** Name: SqlBigInt
**
** Description:
**	Class constructor which establishes a non-NULL value.
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
SqlBigInt( long value )
{
    super( false );
    this.value = value;
} // SqlBigInt


/*
** Name: set
**
** Description:
**	Assign a new non-NULL data value.
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
set( long value )
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
set( SqlBigInt data )
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
**	long	Current value.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public long 
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
    setNotNull();
    this.value = value ? 1L : 0L;
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
    this.value = (long)value;
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
    this.value = (long)value;
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
    this.value = (long)value;
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
    this.value = value;
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
    setNotNull();
    this.value = (long)value;
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
    setNotNull();
    this.value = (long)value;
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
    {
	setNotNull();
	this.value = value.longValue();
    }
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
	
	try { this.value = Long.parseLong( value.trim() ); }
	catch( NumberFormatException ex )
	{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }
    }
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
    return( (value == 0L) ? false : true );
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
    return( (byte)value );
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
    return( (short)value );
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
    return( (int)value );
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
*/

public float 
getFloat() 
    throws SQLException
{
    return( (float)value );
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
    return( (double)value );
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
    return( BigDecimal.valueOf( value ) );
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
*/

public String 
getString() 
    throws SQLException
{
    return( Long.toString( value ) );
} // getString


/*
** Name: getObject
**
** Description:
**	Return the current data value as a Long object.
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
**	Object	Current value converted to Long.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Use more efficient value constructor.
*/

public Object 
getObject() 
    throws SQLException
{
    return( Long.valueOf( value ) );
} // getObject


} // class SqlBigInt
