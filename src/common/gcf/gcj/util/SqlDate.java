/*
** Copyright (c) 2006 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.util;

/*
** Name: SqlDate.java
**
** Description:
**	Defines class which represents an ANSI Date value.
**
**  Classes:
**
**	SqlDate		An ANSI Date value.
**
** History:
**	16-Jun-06 (gordy)
**	    Created.
**      17-Nov-08 (rajus01) SIR 121238
**          Replaced SqlEx references with SQLException or SqlExFactory
**          depending upon the usage of it. SqlEx becomes obsolete to support
**          JDBC 4.0 SQLException hierarchy.
**	24-Dec-08 (gordy)
**	    Use date/time formatter instances.
*/

import	java.util.TimeZone;
import	java.sql.Date;
import	java.sql.Time;
import	java.sql.Timestamp;
import	java.sql.SQLException;


/*
** Name: SqlDate
**
** Description:
**	Class which represents an ANSI Date value.
**
**	Supports conversion to String, Date, Timestamp, and Object.  
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
**	setString		Data value accessor assignment methods
**	setDate
**	setTimestamp
**
**	getString		Data value accessor retrieval methods
**	getDate
**	getTimestamp
**	getObject
**
**  Private Data:
**
**	value			Date value.
**	dates			Date/time formatter.
**
**  Private Methods:
**
**	set			setXXX() helper method.
**	get			getXXX() helper method.
**
** History:
**	16-Jun-06 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Added date/time formatter instance.
*/

public class
SqlDate
    extends SqlData
{

    private String		value = null;
    private SqlDates		dates;
	
    
/*
** Name: SqlDate
**
** Description:
**	Class constructor.  Data value is initially NULL.
**	Default date/time formatter instance is used.
**
** Input:
**	format		Date/time formatter instance.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	16-Jun-06 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Get default date/time formatter instance.
*/

public
SqlDate()
{
    super( true );
    dates = SqlDates.getDefaultInstance();
} // SqlDate


/*
** Name: SqlDate
**
** Description:
**	Class constructor.  Data value is initially NULL.
**	Date/time formatter instance is passed as parameter.
**
** Input:
**	format		Date/time formatter instance.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	24-Dec-08 (gordy)
**	    Created.
*/

public
SqlDate( SqlDates format )
{
    super( true );
    dates = format;
} // SqlDate


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
**	16-Jun-06 (gordy)
**	    Created.
*/

public void
set( String value )
{
    if ( value == null )
	setNull();
    else  
    {
	setNotNull();
	this.value = value;
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
**	16-Jun-06 (gordy)
**	    Created.
*/

public void
set( SqlDate data )
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
**	16-Jun-06 (gordy)
**	    Created.
*/

public String 
get() 
{
    return( value );
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
**	16-Jun-06 (gordy)
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
	Date date;
	    
	/*
	** Validate format by converting to JDBC Date.
	*/
	try { date = Date.valueOf( value ); }
	catch( Exception ex )
	{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }
	    
	setDate( date, null );
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
**	date	Date value (may be null).
**	tz	Relative timezone, NULL for local.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	16-Jun-06 (gordy)
**	    Created.
*/

public void 
setDate( Date date, TimeZone tz ) 
    throws SQLException
{
    set( (java.util.Date)date, tz );
    return;
} // setDate


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
**	ts	Timestamp value (may be null).
**	tz	Relative timezone, NULL for local.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	16-Jun-06 (gordy)
**	    Created.
*/

public void 
setTimestamp( Timestamp ts, TimeZone tz ) 
    throws SQLException
{
    set( (java.util.Date)ts, tz );
    return;
} // setTimestamp


/*
** Name: set
**
** Description:
**	Assign a generic Java Date value as the data value.
**
**	The data value will be NULL if the input value 
**	is null, otherwise non-NULL.
**
**	A relative timezone may be provided which is 
**	applied to adjust the input such that it 
**	represents the original date/time value in the 
**	timezone provided.  The default is to use the 
**	local timezone.
**
** Input:
**	value	Java Date value (may be null).
**	tz	Relative timezone, NULL for local.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	16-Jun-06 (gordy)
**	    Created.
**	24-Jul-07 (rajus01) Bug #111769; Issue # SD 119880
**	    Crossing fix for bug 111769 from edbc.jar codeline.
**	24-Dec-08 (gordy)
**	    Use date/time formatter instance.
*/

private void
set( java.util.Date value, TimeZone tz )
    throws SQLException
{
    if ( value == null )
    {
	setNull();
	return;
    }

    /*
    ** Dates should be independent of TZ, but JDBC date values
    ** are stored in UTC.  Use the TZ provided to ensure the
    ** formatted value represents the date in the desired TZ.
    ** Otherwise, the local default TZ is used.
    */
    setNotNull();
    this.value = (tz != null) ? dates.formatDate( value, tz )
			      : dates.formatDate( value, false );
    /*
    ** Make sure the Date value is in the correct format, i.e yyyy-mm-dd.
    */
    if ( this.value == null  ||  this.value.length() != 10  || 
	 this.value.charAt(4) != '-'  ||  this.value.charAt(7) != '-' )
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
    return;
} // set


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
**	16-Jun-06 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Use date/time formatter instance.
*/

public String 
getString() 
    throws SQLException
{
    /*
    ** Do conversion to check for valid format.
    */
    return( dates.formatDate( get( null ), false ) );
} // getString


/*
** Name: getDate
**
** Description:
**	Return the current data value as a Date value.
**	
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
**	16-Jun-06 (gordy)
**	    Created.
*/

public Date 
getDate( TimeZone tz ) 
    throws SQLException
{ 
    return( get( tz ) );
} // getDate


/*
** Name: getTimestamp
**
** Description:
**	Return the current data value as a Timestamp value.
**
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
**	tz		Relative timezone, NULL for local.
**
** Output:
**	None.
**
** Returns:
**	Timestamp	Current value converted to Timestamp.
**
** History:
**	16-Jun-06 (gordy)
**	    Created.
*/

public Timestamp 
getTimestamp( TimeZone tz ) 
    throws SQLException
{
    /*
    ** Return a Timestamp object with the resulting UTC value.
    */
    return( new Timestamp( get( tz ).getTime() ) );
} // getTimestamp


/*
** Name: getObject
**
** Description:
**	Return the current data value as a Date object.
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
**	Object	Current value converted to Date.
**
** History:
**	16-Jun-06 (gordy)
**	    Created.
*/

public Object 
getObject() 
    throws SQLException
{
    return( get( null ) );
} // getObject


/*
** Name: get
**
** Description:
**	Return the current data value as a Date value.
**	
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
**	16-Jun-06 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Use date/time formatter instance.
*/

private Date
get( TimeZone tz )
    throws SQLException
{
    /*
    ** Dates should be independent of TZ, but JDBC date values
    ** are stored in UTC.  Use the TZ provided to ensure the
    ** resulting UTC value represents the date in the desired 
    ** TZ.  Otherwise, the local default TZ is used.
    */
    return( (tz != null ) ? dates.parseDate( value, tz )
			  : dates.parseDate( value, false ) );
} // get


} // class SqlDate
