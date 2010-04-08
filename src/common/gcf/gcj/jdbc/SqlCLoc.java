/*
** Copyright (c) 2007, 2009 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: SqlCLoc.java
**
** Description:
**	Defines class which represents a Character LOB Locator value.
**
**  Classes:
**
**	SqlCLoc		A Character LOB Locator value.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	30-Jan-07 (gordy)
**	    Backward compatibility for LOB Locators.
**	 4-May-07 (gordy)
**	    Set class access for reflection.
**	24-Dec-08 (gordy)
**	    Use default date/time formatting instance.
**      05-Jan-09 (rajus01) SIR 121238
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to
**            support JDBC 4.0 SQLException hierarchy.
**      13-Apr-09 (rajus01) SIR 121238
**          Added getNlob().
*/

import	java.math.BigDecimal;
import	java.io.InputStream;
import  java.io.Reader;
import	java.util.TimeZone;
import	java.sql.Clob;
import	java.sql.NClob;
import	java.sql.Date;
import	java.sql.Time;
import	java.sql.Timestamp;
import	java.sql.SQLException;
import	com.ingres.gcf.util.SqlLoc;
import	com.ingres.gcf.util.SqlLongChar;
import	com.ingres.gcf.util.SqlStream;
import	com.ingres.gcf.util.SqlDates;
import	com.ingres.gcf.util.SqlExFactory;


/*
** Name: SqlCLoc
**
** Description:
**	Class which represents a Character LOB Locator value.
**
**	Extends the utility SqlLoc class to add the ability to
**	convert the locator into supported JDBC data types.
**	Supports conversion to Clob and Object.  For backward
**	compatibility, additional accessor retrieval methods
**	are supported to match functionality of SqlLongChar.
**
**	The data value accessor methods do not check for the NULL 
**	condition.  The super-class isNull() method should first 
**	be checked prior to calling any of the accessor methods.
**
**  Public Methods:
**
**	set			Assign a new data value.
**
**	setClob			Data value accessor assignment methods.
**
**	getClob			Data value accessor retrieval methods.
**	getBoolean
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
**  Private Data:
**
**	conn			DBMS connection.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	30-Jan-07 (gordy)
**	    Added stream accessor retrieval methods getBoolean(), getByte(),
**	    getShort(), getInt(), getLong(), getFloat(), getDouble(),
**	    getBigDecimal(), getString, getDate(), getTime(), getTimestamp(),
**	    getAsciiStream(), getUnicodeStream(), getCharacterStream().
**	 4-May-07 (gordy)
**	    Class is not exposed outside package, restrict access.
*/

class	// package access
SqlCLoc
    extends SqlLoc
{

    private DrvConn		conn = null;


/*
** Name: SqlCLoc
**
** Description:
**	Class constructor.  Data value is initially NULL.
**
** Input:
**	conn	DBMS connection.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public
SqlCLoc( DrvConn conn )
    throws SQLException
{
    super();
    this.conn = conn;
} // SqlCLoc


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
**	15-Nov-06 (gordy)
**	    Created.
*/

public void
set( SqlCLoc data )
{
    if ( data == null )
	setNull();
    else
    {
	conn = data.conn;
	set( (SqlLoc)data );
    }
    return;
} // set


/*
** Name: setClob
**
** Description:
**	Assign a Clob object as the non-NULL data value.
**
**	While the generic interface allows any Clob object
**	to be assigned, this method only supports DrvClob
**	objects.
**	
** Input:
**	value	Clob object.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public void
setClob( Clob value ) 
    throws SQLException
{ 
    if ( value == null )
	setNull();
    else  if ( value instanceof DrvClob )
	set( ((DrvClob)value).getLocator() );
    else
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); 

    return;
} // setClob


/*
** Name: getClob
**
** Description:
**	Returns a Clob wrapper for the LOB Locator
**	which can access the LOB data in the database.
**	The Clob wrapper can also manage the LOB as
**	buffered LOB data.
**	
**      Note: the value returned when the data value is
**      NULL is not well defined.
**
** Input:
**	None.
**	
** Output:
**	None.
**
** Returns:
**	Clob	Clob wrapper for LOB Locator.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	26-Feb-07 (gordy)
**	    Added capability to configure cache segment size.
*/

public Clob
getClob()
    throws SQLException
{
    /*
    ** The LOB Locator is first wrapped in a class
    ** which is able to access the LOB data in the
    ** database.  The locator Clob is then wrapped 
    ** in a class which can convert and manage the
    ** LOB as updatable buffered data.
    */
    return( new JdbcClob( new DrvClob( conn, value ), 
			  conn.cnf_lob_segSize, conn.trace ) );
} // getClob

/*
** Name: getNlob
**
** Description:
**	Returns a NClob wrapper for the LOB Locator
**	which can access the LOB data in the database.
**	The Clob wrapper can also manage the LOB as
**	buffered LOB data.
**	
**      Note: the value returned when the data value is
**      NULL is not well defined.
**
** Input:
**	None.
**	
** Output:
**	None.
**
** Returns:
**	NClob	NClob wrapper for LOB Locator.
**
** History:
**	10-Apr-09 (rajus01)
**	    Created.
*/
public NClob
getNlob()
    throws SQLException
{
    /*
    ** The LOB Locator is first wrapped in a class
    ** which is able to access the LOB data in the
    ** database.  The locator Clob is then wrapped 
    ** in a class which can convert and manage the
    ** LOB as updatable buffered data.
    */
    return( new JdbcNlob( new DrvClob( conn, value ), 
			  conn.cnf_lob_segSize, conn.trace ) );
} // getNlob

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
**	30-Jan-07 (gordy)
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
**	30-Jan-07 (gordy)
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
**	30-Jan-07 (gordy)
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
**	30-Jan-07 (gordy)
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
**	30-Jan-07 (gordy)
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
**	30-Jan-07 (gordy)
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
**	30-Jan-07 (gordy)
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
**	30-Jan-07 (gordy)
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
**	String	Current value converted to String.
**
** History:
**	30-Jan-07 (gordy)
**	    Created.
*/

public String
getString()
    throws SQLException
{
    return( SqlLongChar.strm2str( getCharacterStream() ) );
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
**	30-Jan-07 (gordy)
**	    Created.
*/

public String
getString( int limit )
    throws SQLException
{
    return( SqlLongChar.strm2str( getCharacterStream(), limit ) );
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
**	30-Jan-07 (gordy)
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
**	30-Jan-07 (gordy)
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
**	30-Jan-07 (gordy)
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
**      Note: the value returned when the data value is
**      NULL is not well defined.
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
**	30-Jan-07 (gordy)
**	    Created.
**      03-Feb-09 (rajus01) SIR 121238
**          Code cleanup: Removed try...catch() block as SqlEx is gone.
*/

public InputStream
getAsciiStream()
    throws SQLException
{
    /*
    ** Generate Clob wrapper which provides stream access.
    */
    InputStream	is;
    Clob	clob = new DrvClob( conn, value );

    is = clob.getAsciiStream();

    return( is );
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
**	30-Jan-07 (gordy)
**	    Created.
*/

public InputStream
getUnicodeStream()
    throws SQLException
{
    return( SqlStream.getUnicodeIS( getCharacterStream() ) );
} // getUnicodeStream


/*
** Name: getCharacterStream
**
** Description:
**	Return the current data value as an character stream.
**
**      Note: the value returned when the data value is
**      NULL is not well defined.
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
**	30-Jan-07 (gordy)
**	    Created.
**      03-Feb-09 (rajus01) SIR 121238
**          Code cleanup: Removed try...catch() block as SqlEx is gone.
*/

public Reader
getCharacterStream()
    throws SQLException
{
    /*
    ** Generate Clob wrapper which provides stream access.
    */
    Reader	rdr;
    Clob	clob = new DrvClob( conn, value );

    rdr = clob.getCharacterStream();

    return( rdr );
} // getCharacterStream


/*
** Name: getObject
**
** Description:
**      Return the current data value as an Binary object.
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
**      Object  Current value converted to Binary.
**
** History:
**      15-Nov-06 (gordy)
**          Created.
*/

public Object
getObject()
    throws SQLException
{
    return( getClob() );
} // getObject


} // class SqlCLoc

