/*
** Copyright (c) 2006, 2009 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.util;

/*
** Name: SqlTimestamp.java
**
** Description:
**	Defines class which represents an ANSI Timestamp value.
**
**  Classes:
**
**	SqlTimestamp	An ANSI Timestamp value.
**
** History:
**	19-Jun-06 (gordy)
**	    Created.
**	22-Sep-06 (gordy)
**	    WITH TZ variant is now in 'local' time rather than
**	    UTC time.  Added external access to nano-seconds.
**      17-Nov-08 (rajus01) SIR 121238
**          Replaced SqlEx references with SQLException as SqlEx becomes
**          obsolete to support JDBC 4.0 SQLException hierarchy.
**	24-Dec-08 (gordy)
**	    Use date/time formatter instances.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
*/

import	java.util.TimeZone;
import	java.sql.Date;
import	java.sql.Time;
import	java.sql.Timestamp;
import	java.sql.SQLException;


/*
** Name: SqlTimestamp
**
** Description:
**	Class which represents an ANSI Timestamp value.  Three 
**	variants are supported:
**
**	WITHOUT TIME ZONE
**	    Timezone independent.  Since local storage is UTC, an
**	    external TZ or the local default TZ is used for 
**	    formatting/parsing.
**
**	LOCAL TIME ZONE
**	    Represents local time stored in UTC.  GMT is used for 
**	    formatting/parsing.
**
**	WITH TIME ZONE
**	    UTC with explicit timezone offset.  GMT is used for
**	    formatting/parsing.  An external TZ or the local
**	    default TZ is used to determine the timezone offset.
**
**	Fractional seconds are supported.  Due to the how fractional
**	seconds are supported in java.util.Date, java.sql.Date, and 
**	the date formatters, fractional seconds are stored as a 
**	separate nano-second value.
**
**	Supports conversion to String, Date, Time, Timestamp, and Object.  
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
**	setTime
**	setTimestamp
**
**	getString		Data value accessor retrieval methods
**	getDate
**	getTime
**	getTimestamp
**	getObject
**
**	getNanos		Get franctional seconds (nano-seconds).
**
**  Private Data:
**
**	value			Date value.
**	nanos			Fractional seconds expressed in nano-seconds.
**	timezone		Associated timezone.
**	dbms_type		DBMS specific variat.
**	dates			Date/time formatter.
**
**  Private Methods:
**
**	set			setXXX() helper method.
**	get			getXXX() helper method.
**
** History:
**	19-Jun-06 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Added date/time formatter instance.
*/

public class
SqlTimestamp
    extends SqlData
    implements DbmsConst
{

    private String		value = null;
    private int			nanos = 0;
    private String		timezone = null;
    private short		dbms_type = DBMS_TYPE_TS;
    private SqlDates		dates;
	
    
/*
** Name: SqlTimestamp
**
** Description:
**	Class constructor.  Data value is initially NULL.
**	Default date/time formatter instance is used.
**
** Input:
**	dbms_type	DBMS specific variant.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	19-Jun-06 (gordy)
**	    Created.
**	22-Sep-06 (gordy)
**	    Validate DBMS type.
**	24-Dec-08 (gordy)
**	    Get default date/time formatter instance.
*/

public
SqlTimestamp( short dbms_type )
    throws SQLException
{
    super( true );

    switch( dbms_type )
    {
    case DBMS_TYPE_TS :
    case DBMS_TYPE_TSWO :
    case DBMS_TYPE_TSTZ :
	this.dbms_type = dbms_type;
	break;

    default :	/* Should not happen! */
	throw SqlExFactory.get( ERR_GC401B_INVALID_DATE );
    }

    dates = SqlDates.getDefaultInstance();

} // SqlTimestamp


/*
** Name: SqlTimestamp
**
** Description:
**	Class constructor.  Data value is initially NULL.
**	Date/time formatter instance is passed as parameter.
**
** Input:
**	format		Date/time formatter instance.
**	dbms_type	DBMS specific variant.
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
SqlTimestamp( SqlDates format, short dbms_type )
    throws SQLException
{
    super( true );

    switch( dbms_type )
    {
    case DBMS_TYPE_TS :
    case DBMS_TYPE_TSWO :
    case DBMS_TYPE_TSTZ :
	this.dbms_type = dbms_type;
	break;

    default :	/* Should not happen! */
	throw SqlExFactory.get( ERR_GC401B_INVALID_DATE );
    }

    dates = format;

} // SqlTimestamp


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
**	19-Jun-06 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Use date/time formatter instance.
*/

public void
set( String value )
    throws SQLException
{
    if ( value == null )
	setNull();
    else  
    {
	/*
	** Separate explicit timezone.
	*/
	if ( dbms_type == DBMS_TYPE_TSTZ )
	{
	    if ( value.length() < 
		 (SqlDates.TS_FMT.length() + SqlDates.TZ_FMT.length()) )
		throw SqlExFactory.get( ERR_GC401B_INVALID_DATE );
    
	    int offset = value.length() - SqlDates.TZ_FMT.length();
	    timezone = value.substring( offset );
	    value = value.substring( 0, offset );
	}

	/*
	** Separate fractional seconds.
	*/
	if ( value.length() > SqlDates.TS_FMT.length() )
	{
	    nanos = dates.parseFrac(value.substring(SqlDates.TS_FMT.length()));
	    value = value.substring( 0, SqlDates.TS_FMT.length() );
	}

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
**	19-Jun-06 (gordy)
**	    Created.
**	15-Nov-06 (gordy)
**	    Copy the DBMS specific varient of the new data value.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Move parameter dereference after null check.
*/

public void
set( SqlTimestamp data )
{
    if ( data == null  ||  data.isNull() )
	setNull();
    else
    {
	setNotNull();
	value = data.value;
	nanos = data.nanos;
	timezone = data.timezone;
	dbms_type = data.dbms_type;
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
**	19-Jun-06 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Use date/time formatter instance.
*/

public String 
get() 
{
    String value = this.value;

    if ( nanos > 0 )  value += dates.formatFrac( nanos );
    if ( dbms_type == DBMS_TYPE_TSTZ )  value += timezone;

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
**	19-Jun-06 (gordy)
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
	Timestamp ts;
		
	/*
	** Validate format by converting to JDBC Timestamp.
	*/
	try { ts = Timestamp.valueOf( value ); }
	catch( Exception ex_t )
	{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }
		
	setTimestamp( ts, null );
    }
    
    return;
} // setString


/*
** Name: setDate
**
** Description:
**	Assign a Date value as the data value.
**
**	The data value will be NULL if the input value 
**	is null, otherwise non-NULL.
**
**	A relative timezone may be provided which is 
**	applied to non-UTC time values to adjust the 
**	input such that it represents the original 
**	date/time value in the timezone provided.
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
**	19-Jun-06 (gordy)
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
** Name: setTime
**
** Description:
**	Assign a Time value as the data value.
**
**	The data value will be NULL if the input value 
**	is null, otherwise non-NULL.
**
**	A relative timezone may be provided which is 
**	applied to non-UTC time values to adjust the 
**	input such that it represents the original 
**	date/time value in the timezone provided.
**
** Input:
**	time	Time value (may be null).
**	tz	Relative timezone, NULL for local.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	19-Jun-06 (gordy)
**	    Created.
*/

public void 
setTime( Time time, TimeZone tz ) 
    throws SQLException
{
    set( (java.util.Date)time, tz );
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
**	A relative timezone may be provided which is 
**	applied to non-UTC time values to adjust the 
**	input such that it represents the original 
**	date/time value in the timezone provided.
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
**	19-Jun-06 (gordy)
**	    Created.
*/

public void 
setTimestamp( Timestamp ts, TimeZone tz ) 
    throws SQLException
{
    set( (java.util.Date)ts, tz );
    if ( ts != null )  nanos = ts.getNanos();
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
**	Functionally, only the raw offset of the Date value
**	is used, so the actual source value may be a JDBC 
**	Date, Time or Timestamp object.  Since java Dates
**	only support milli-seconds, the associated nano-
**	second value is set to zero.
**
**	A relative timezone may be provided which is 
**	applied to non-UTC time values to adjust the 
**	input such that it represents the original 
**	date/time value in the timezone provided.
**
** Input:
**	value	Java Date value (may be null).
**	tz	Relative timezone (null for local).
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	19-Jun-06 (gordy)
**	    Created.
**	22-Sep-06 (gordy)
**	    WITH TIME ZONE format now uses the local time value.
**    24-Jul-07 (rajus01) Bug #111769; Issue # SD 119880
**        Crossing fix for bug 111769 from edbc.jar codeline.
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

    setNotNull();
    nanos = 0;

    switch( dbms_type )
    {
    case DBMS_TYPE_TS :
	/*
	** DAS parses local time using GMT.
	*/
	this.value = dates.formatTimestamp( value, true );
	break;

    case DBMS_TYPE_TSWO :
	/*
	** Format as local time using requested or default timezone.
	*/
	this.value = (tz != null) ? dates.formatTimestamp( value, tz ) 
				  : dates.formatTimestamp( value, false );
	break;

    case DBMS_TYPE_TSTZ :
	/*
	** Format as local time using requested or default timezone.
	*/
	this.value = (tz != null) ? dates.formatTimestamp( value, tz ) 
				  : dates.formatTimestamp( value, false );

	/*
	** Get the TZ offset of the target value in the requested
	** or default timezone.
	*/
	timezone = (tz != null) ? dates.formatTZ( tz, value )
				: dates.formatTZ( value );
	break;
    }

    /*
    ** A valid Timestamp value is in the format:
    ** yyyy-mm-dd hh:mm:ss.fffffffff
    */
    if( this.value == null || this.value.length() < 19 ||
	this.value.length() > 29 || this.value.charAt(10) != ' ' || 
	this.value.charAt(13) != ':' || this.value.charAt(16) != ':' )
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
**	19-Jun-06 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Use date/time formatter instance.
*/

public String 
getString() 
    throws SQLException
{
    /*
    ** Do conversion to validate format.
    ** Format using local default TZ for local time.
    ** Nano-seconds must be manually formatted.
    */
    String str = dates.formatTimestamp( get( null ), false );
    if ( nanos > 0 )  str += dates.formatFrac( nanos );

    return( str );
} // getString


/*
** Name: getDate
**
** Description:
**	Return the current data value as a Date value.
**
**	A relative timezone may be provided which is
**	applied to non-UTC time values to adjust the 
**	final result such that it represents the 
**	original date/time value in the timezone 
**	provided.  The default is to use the local 
**	timezone.
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
**	Date	Current value converted to Date.
**
** History:
**	19-Jun-06 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Use date/time formatter instance.
*/

public Date 
getDate( TimeZone tz ) 
    throws SQLException
{ 
    /*
    ** Strip time component by re-formatting as date value.
    */
    return( dates.parseDate( dates.formatDate(get(tz), false), false ) );
} // getDate


/*
** Name: getTime
**
** Description:
**	Return the current data value as a Time value.
**
**	A relative timezone may be provided which is
**	applied to non-UTC time values to adjust the 
**	final result such that it represents the 
**	original date/time value in the timezone 
**	provided.  The default is to use the local 
**	timezone.
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
**	19-Jun-06 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Use date/time formatter instance.
*/

public Time 
getTime( TimeZone tz ) 
    throws SQLException
{ 
    /*
    ** Strip date component by re-formatting as time value.
    */
    return( dates.parseTime( dates.formatTime(get(tz), false), false ) );
} // getTime


/*
** Name: getTimestamp
**
** Description:
**	Return the current data value as a Timestamp value.
**
**	A relative timezone may be provided which is
**	applied to non-UTC time values to adjust the 
**	final result such that it represents the 
**	original date/time value in the timezone 
**	provided.  The default is to use the local 
**	timezone.
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
**	19-Jun-06 (gordy)
**	    Created.
*/

public Timestamp 
getTimestamp( TimeZone tz ) 
    throws SQLException
{
    return( get( tz ) );
} // getTimestamp


/*
** Name: getObject
**
** Description:
**	Return the current data value as a Timestamp object.
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
**	Object	Current value converted to Timestamp.
**
** History:
**	19-Jun-06 (gordy)
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
**	Return the current data value as a Timestamp value.
**
**	A relative timezone may be provided which is
**	applied to non-UTC time values to adjust the 
**	final result such that it represents the 
**	original date/time value in the timezone 
**	provided.  The default is to use the local 
**	timezone.
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
**	19-Jun-06 (gordy)
**	    Created.
**	22-Sep-06 (gordy)
**	    WITH TIME ZONE format now uses the local time value.
**	24-Dec-08 (gordy)
**	    Use date/time formatter instance.
*/

private Timestamp
get( TimeZone tz )
    throws SQLException
{
    Timestamp ts = null;

    switch( dbms_type )
    {
    case DBMS_TYPE_TS : 
	/*
	** DAS formats local time using GMT.
	*/
	ts = dates.parseTimestamp( value, true );
	break;

    case DBMS_TYPE_TSWO : 
	/*
	** Interpret as local time in requested or default timezone.
	*/
	ts = (tz != null) ? dates.parseTimestamp( value, tz )
			  : dates.parseTimestamp( value, false );
	break;

    case DBMS_TYPE_TSTZ :
	/*
	** Apply explicit timezone.
	*/
	ts = dates.parseTimestamp( value, dates.getTZ( timezone ) );
	break;
    }

    ts.setNanos( nanos );
    return( ts );
} // get


/*
** Name: getNanos
**
** Description:
**	Get the timestamps fractional seconds (nano-second) component.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Return:
**	int	Nano-seconds.
**
** History:
**	22-Sep-06 (gordy)
**	    Created.
*/

public int
getNanos()
{
    return( nanos );
} // getNanos()


} // class SqlTimestamp
