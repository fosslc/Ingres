/*
** Copyright (c) 2006, 2009 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.util;

/*
** Name: SqlTime.java
**
** Description:
**	Defines class which represents an ANSI Time value.
**
**  Classes:
**
**	SqlTime		An ANSI Time value.
**
** History:
**	19-Jun-06 (gordy)
**	    Created.
**	22-Sep-06 (gordy)
**	    WITH TZ variant is now in 'local' time rather than UTC time.
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
** Name: SqlTime
**
** Description:
**	Class which represents an ANSI Time value.  Three variants
**	are supported:
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
**	    formatting/parsing.  An explicit TZ or the local 
**	    default TZ is used to determine the timezone offset.
**
**	Fractional seconds are ignored since they are not supported 
**	by the JDBC Time class.
**
**	Supports conversion to String, Time, Timestamp, and Object.  
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
**	setTime
**	setTimestamp
**
**	getString		Data value accessor retrieval methods
**	getTime
**	getTimestamp
**	getObject
**
**  Private Data:
**
**	value			Date value.
**	timezone		Associated timezone.
**	dbms_type		DBMS specific variat.
**	use_gmt			Use GMT for DBMS date/time values.
**	dates			Date/time formatter.
**
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
SqlTime
    extends SqlData
    implements DbmsConst
{

    private String		value = null;
    private String		timezone = null;
    private short		dbms_type = DBMS_TYPE_TIME;
    private SqlDates		dates;
	
    
/*
** Name: SqlTime
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
SqlTime( short dbms_type )
    throws SQLException
{
    super( true );

    switch( dbms_type )
    {
    case DBMS_TYPE_TIME :
    case DBMS_TYPE_TMWO :
    case DBMS_TYPE_TMTZ :
	this.dbms_type = dbms_type;
	break;

    default :	/* Should not happen! */
	throw SqlExFactory.get( ERR_GC401B_INVALID_DATE );
    }

    dates = SqlDates.getDefaultInstance();

} // SqlTime


/*
** Name: SqlTime
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
SqlTime( SqlDates format, short dbms_type )
    throws SQLException
{
    super( true );

    switch( dbms_type )
    {
    case DBMS_TYPE_TIME :
    case DBMS_TYPE_TMWO :
    case DBMS_TYPE_TMTZ :
	this.dbms_type = dbms_type;
	break;

    default :	/* Should not happen! */
	throw SqlExFactory.get( ERR_GC401B_INVALID_DATE );
    }

    dates = format;

} // SqlTime


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
	if ( dbms_type == DBMS_TYPE_TMTZ )
	{
	    if ( value.length() < 
		 (SqlDates.T_FMT.length() + SqlDates.TZ_FMT.length()) )
		throw SqlExFactory.get( ERR_GC401B_INVALID_DATE );
    
	    int offset = value.length() - SqlDates.TZ_FMT.length();
	    timezone = value.substring( offset );
	    value = value.substring( 0, offset );
	}

	/*
	** Remove fractional seconds.
	*/
	if ( value.length() > SqlDates.T_FMT.length() )  
	    value = value.substring( 0, SqlDates.T_FMT.length() );

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
**	    Copy the DBMS specific variant of the new data value.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Move parameter dereference after null check.
*/

public void
set( SqlTime data )
{
    if ( data == null  ||  data.isNull() )
	setNull();
    else
    {
	setNotNull();
	value = data.value;
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
*/

public String 
get() 
{
    return( (dbms_type == DBMS_TYPE_TMTZ) ? value + timezone : value );
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
	Time time;
		
	/*
	** Validate format by converting to JDBC Time.
	*/
	try { time = Time.valueOf( value ); }
	catch( Exception ex_t )
	{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }
		
	setTime( time, null );
    }
    
    return;
} // setString


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
**	19-Jun-06 (gordy)
**	    Created.
*/

public void 
setTime( Time value, TimeZone tz ) 
    throws SQLException
{
    set( (java.util.Date)value, tz );
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
**	19-Jun-06 (gordy)
**	    Created.
*/

public void 
setTimestamp( Timestamp value, TimeZone tz ) 
    throws SQLException
{
    set( (java.util.Date)value, tz );
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
**	Time or Timestamp object.
**
**	A relative timezone may be provided which is 
**	applied to non-UTC time values to adjust the 
**	input such that it represents the original 
**	date/time value in the timezone provided.
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
**	19-Jun-06 (gordy)
**	    Created.
**	22-Sep-06 (gordy)
**	    WITH TIME ZONE format now uses the local time value.
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

    setNotNull();

    switch( dbms_type )
    {
    case DBMS_TYPE_TIME :
	/*
	** DAS parses local time using GMT.
	*/
	this.value = dates.formatTime( value, true );
	break;

    case DBMS_TYPE_TMWO :
	/*
	** Format as local time using requested or default timezone.
	*/
	this.value = (tz != null) ? dates.formatTime( value, tz ) 
				  : dates.formatTime( value, false );
	break;

    case DBMS_TYPE_TMTZ :
	/*
	** Format as local time using requested or default timezone.
	*/
	this.value = (tz != null) ? dates.formatTime( value, tz ) 
				  : dates.formatTime( value, false );

	/*
	** Java applies TZ and DST of 'epoch' date: 1970-01-01.
	** Ingres applies TZ and DST of todays date, so use 
	** current date to determine explicit TZ offset.
	*/
	long millis = System.currentTimeMillis();
	timezone = (tz != null) ? dates.formatTZ( tz, millis )
				: dates.formatTZ( millis );
	break;
    }
    
    /* Valid Time value is in the format "hh:mm:ss" */
    if( this.value == null || this.value.length() != 8 ||
	this.value.charAt(2) != ':' || this.value.charAt(5) != ':' )
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
    ** Format using local default TZ for local time.
    */
    return( dates.formatTime( get( null ), false ) );
} // getString


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
*/

public Time 
getTime( TimeZone tz ) 
    throws SQLException
{ 
    return( get( tz ) );
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
    /*
    ** Return a Timestamp object with resulting UTC value.
    */
    return( new Timestamp( get( tz ).getTime() ) );
} // getTimestamp


/*
** Name: getObject
**
** Description:
**	Return the current data value as a Time object.
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
**	Object	Current value converted to Time.
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
**	22-Sep-06 (gordy)
**	    WITH TIME ZONE format now uses the local time value.
**	    Handle DST and day boundaries with UTC time.
**	24-Dec-08 (gordy)
**	    Use date/time formatter instance.
*/

/*
** This static storage allows updates of the current
** date/time without creating a lot of objects.
*/
private static java.util.Date	now = new java.util.Date();

private Time
get( TimeZone tz )
    throws SQLException
{
    Time time = null;

    switch( dbms_type )
    {
    case DBMS_TYPE_TIME :
    {
	if ( tz == null )  tz = TimeZone.getDefault();
	now.setTime( System.currentTimeMillis() );

	/*
	** DAS formats local time using GMT.  Normally,
	** this would be a straight conversion into UTC
	** Java storage, but two problems exist.  
	**
	** Ingres bases DST offsets on the current date 
	** while Java uses the date 'epoch' 1970-01-01,
	** which is not in DST.  
	**
	** Time is supposed to be assigned the 'epoch'
	** date in default/requested timezone, but UTC
	** may be in a different day.
	*/

	/*
	** Check if current date is in DST.
	*/
	if ( ! tz.useDaylightTime()  ||  ! tz.inDaylightTime( now ) )
	    time = dates.parseTime( value, true );
	else
	{
	    /*
	    ** Adjust UTC time to offset DST difference.
	    */
	    int dst = tz.getDSTSavings() / 60000;
	    time = dates.parseTime( value, dates.getTZ( -dst ) );
	}

	long utc = time.getTime();
	long day = 24 * 60 * 60 * 1000;

	/*
	** Make sure UTC time is in the 'epoch' day
	** (should always be true, but check anyway).
	*/
	if ( utc >= 0  &&  utc < day )
	{
	    /*
	    ** See if target TZ is in different day
	    ** and adjust time accordingly.
	    */
	    if ( (utc + tz.getRawOffset()) < 0 )
		time.setTime( utc + day );
	    else  if ( (utc + tz.getRawOffset()) >= day )
		time.setTime( utc - day );
	}
	break;
    }
    case DBMS_TYPE_TMWO :
	/*
	** Interpret as local time using requested or default timezone.
	*/
    	time = (tz != null) ? dates.parseTime( value, tz )
			    : dates.parseTime( value, false );
	break;

    case DBMS_TYPE_TMTZ :
	/*
	** Time parameters are formatted with a TZ which 
	** represents todays DST setting. Java time values
	** are associated with the date 'epoch' 1970-01-01
	** which is always non-DST.  To compensate for the
	** difference, the fixed TZ is adjusted by the DST 
	** offset of the requested or default timezone for 
	** the current date.
	**
	** As a result, the value will differ based on the
	** DST of the date accessed.  The result is some-
	** what different than native access for this type 
	** (which shows the same time and an explicit time
	** zone offset), but is similar to what occurs with
	** LOCAL TIME ZONE values.  Most important, however,
	** is that input and output values in the same DST
	** context will be the same.
	*/
	if ( tz == null )  tz = TimeZone.getDefault();
	now.setTime( System.currentTimeMillis() );

	if ( ! tz.useDaylightTime()  ||  ! tz.inDaylightTime( now ) )
	    time = dates.parseTime( value, dates.getTZ( timezone ) );
	else
	{
	    int dst = tz.getDSTSavings();
	    tz = dates.getTZ( timezone );
	    time = dates.parseTime( value, 
			dates.getTZ( (tz.getRawOffset() - dst) / 60000 ) );
	}
	break;
    }

    return( time );
} // get


} // class SqlTime
