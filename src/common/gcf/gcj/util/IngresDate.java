/*
** Copyright (c) 2003, 2008 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.util;

/*
** Name: IngresDate.java
**
** Description:
**	Defines class which represents an Ingres Date value.
**
**  Classes:
**
**	IngresDate	An Ingres Date value.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Added support for parameter types/values in addition to 
**	    existing support for columns.
**	19-Jun-06 (gordy)
**	    ANSI Date/Time data type support.
**	12-Sep-07 (gordy)
**	    Added capability to define how empty dates are handled.
**      17-Nov-08 (rajus01) SIR 121238
**          Replaced SqlEx references with SQLException or SqlExFactory
**          depending upon the usage of it. SqlEx becomes obsolete to support
**          JDBC 4.0 SQLException hierarchy.
**	24-Dec-08 (gordy)
**	    Use date/time formatter instances.
**      13-Oct-10 (rajus01) SIR 124588, SD issue 147074
**	    Added ability to store blank date back in the DBMS.
**
**	    Background: The driver can be configured to have replacement
**	    value for empty date/time/timestamps returned from the DBMS.
**	    An empty date retrieved is converted into the replacement value
**	    by the driver and this value is now sent back into the driver
**	    by the hibernate ORM framework and hence the requirement to
**	    convert the replacement value back into an empty date.
**
**	    The possible replacement values as defined by the system
**	    property ingres.jdbc.date.empty are: "", 'default', 'empty',
**	    'null' or a valid JDBC date/time value such as
**	    '9999-12-31 23:59:59'.
**
**	    The setNull() method is implemented to handle the case when
**	    when empty date replacement is set to null. Also the isNull()
**	    method no longer required. All of the setNull() methods are
**	    replaced with super.setNull(). The get() method returns a 
**	    non-zero empty string for empty dates.
**
**	    When values are retrieved from the database, setNull() and
**	    set( String ) are used to store the column value and isNull() and
**	    getXXX() are used to return the value to the application.
**	    When parameters are passed to the server, setNull() and setXXX()
**	    are used to store the value and isNull() and get() are used to
**	    pass the value to the server.
**
**	    While testing the ability to store the blank date back in the
**	    database it came to light there is a basic conflict in how
**	    setNull()/isNull() are used when the empty date is configured
**	    as null. For consistent behavior, the requirements when empty 
**	    date configured as null are changed
**
**          From:
**
**	    for Column values
**
**          setNull()   : set IngresDate NULL
**          set("")     : set IngresDate EMPTY
**          isNull()    : returns TRUE for both cases
**
**          for Parameters
**
**          setNull()   : set IngresDate EMPTY
**          setXXX(null): set IngresDate EMPTY
**          isNull()    : returns FALSE
**          get()       : returns " "
**
**         To:
**
**         for Column values
**
**          set(null)   : set IngresDate NULL
**          set("")     : set IngresDate NULL
**
**         for Parameters
**          setNull()   : set IngresDate EMPTY
**          setXXX(null): set IngresDate EMPTY
**          get()       : returns " "
**
**	    IMPORTANT: DON'T choose null replacement for empty dates because
**	    actual null values WILL NOT work through the driver and the 
**	    actual null values will be replaced by EMPTY value. Therefore
**	    null replacement for empty dates is NOT recommended.
*/

import	java.util.TimeZone;
import	java.sql.Date;
import	java.sql.Time;
import	java.sql.Timestamp;
import	java.sql.SQLException;


/*
** Name: IngresDate
**
** Description:
**	Class which represents an Ingres Date value.  Ingres Date
**	values combine aspects of SQL Date, Time, and Timestamp
**	values while also supporting empty dates and intervals.
**
**	Supports conversion to String, Date, Time, Timestamp, and 
**	Object.  
**
**	The data value accessor methods do not check for the NULL
**	condition.  The super-class isNull() method should first
**	be checked prior to calling any of the accessor methods.
**
**	Ingres dates are truncatable.  Both empty dates and date
**	only values are considered truncated.  A truncated Ingres
**	date produces date/time epoch values where applicable.
**
**  Public Methods:
**
**	set			Assign a new data value.
**	get			Retrieve current data value.
**	isNull			Data value is NULL?
**	isInterval		Data value is an interval?
**	isTruncated		Data value is truncated?
**	getDataSize		Expected size of data value.
**	getTruncSize		Actual size of truncated data value.
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
**  Protected Data:
**
**	value			Date value.
**	empty_date		Value used for empty dates.
**	osql_dates		Use OpenSQL date semantics.
**	use_gmt			Use GMT for DBMS date/time values.
**	interval		Is value an interval (valid after getXXX()).
**	dates			Date/time formatter.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Added parameter data value oriented methods.
**	19-Jun-06 (gordy)
**	    Added osql_dates to better differentiate correct behaviour.
**	12-Sep-07 (gordy)
**	    Added replacement string for empty dates and constructor
**	    which allows definition of the replacement string.  Over-
**	    ride isNull() to handle NULL replacement for empty dates.
**	24-Dec-08 (gordy)
**	    Added date/time formatter instance.
*/

public class
IngresDate
    extends SqlData
{

    private String		value = null;
    private String		empty_date = "";
    private boolean		osql_dates = false;
    private boolean		use_gmt = false;
    private boolean		interval = false;
    private SqlDates		dates;
	
    
/*
** Name: IngresDate
**
** Description:
**	Class constructor.  Data value is initially NULL.
**	Default date/time formatter instance is used.
**
** Input:
**	osql_dates	Use OpenSQL date semantics
**	use_gmt		Use GMT for DBMS date/time values.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	19-Jun-06 (gordy)
**	    Added osql_dates to better differentiate correct behaviour.
**	24-Dec-08 (gordy)
**	    Get default date/time formatter instance.
*/

public
IngresDate( boolean osql_dates, boolean use_gmt )
{
    super( true );
    this.osql_dates = osql_dates;
    this.use_gmt = use_gmt;
    dates = SqlDates.getDefaultInstance();
} // IngresDate


/*
** Name: IngresDate
**
** Description:
**	Class constructor.  Data value is initially NULL.
**	Date/time formatter instance is passed as parameter.
**
** Input:
**	format		Date/time formatter instance.
**	osql_dates	Use OpenSQL date semantics
**	use_gmt		Use GMT for DBMS date/time values.
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
IngresDate( SqlDates format, boolean osql_dates, boolean use_gmt )
{
    super( true );
    this.osql_dates = osql_dates;
    this.use_gmt = use_gmt;
    dates = format;
} // IngresDate


/*
** Name: IngresDate
**
** Description:
**	Class constructor permitting empty date replacement string
**	to be defined.  Data value is initially NULL.
**	Default date/time formatter instance is used.
**
** Input:
**	osql_dates	Use OpenSQL date semantics
**	use_gmt		Use GMT for DBMS date/time values.
**	empty_date	Empty date replacement string, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	12-Sep-07 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Get default date/time formatter instance.
*/

public
IngresDate( boolean osql_dates, boolean use_gmt, String empty_date )
{
    this( osql_dates, use_gmt );
    this.empty_date = empty_date;
} // IngresDate


/*
** Name: IngresDate
**
** Description:
**	Class constructor permitting empty date replacement string
**	to be defined.  Data value is initially NULL.
**	Date/time formatter instance is passed as parameter.
**
** Input:
**	format		Date/time formatter instance.
**	osql_dates	Use OpenSQL date semantics
**	use_gmt		Use GMT for DBMS date/time values.
**	empty_date	Empty date replacement string, may be NULL.
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
IngresDate
( 
    SqlDates	format,
    boolean	osql_dates, 
    boolean	use_gmt,
    String	empty_date 
)
{
    this( format, osql_dates, use_gmt );
    this.empty_date = empty_date;
} // IngresDate


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
**	12-Sep-03 (gordy)
**	    Created.
*/

public void
set( String value )
{
    if ( value == null ||  (value.length() == 0  &&  empty_date == null) )
	super.setNull();
    else  
    {
	setNotNull();
	this.value = value;
	interval = false;
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
set( IngresDate data )
{
    if ( data == null  ||  data.isNull() )
	super.setNull();
    else
    {
	setNotNull();
	value = data.value;
	interval = data.interval;
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
    /*
    ** DAS treats the value passed for an Ingres date as a DB_CHA_TYPE.
    ** That string type does not permit 0 length values, so for empty
    ** dates return a non-zero empty string instead.
    */
    return( (value != null  &&  value.length() == 0) ? " " : value );

} // get

/*
** Name: isInterval
**
** Description:
**	Returns an indication that the data value is an interval.
**
**	Note that since intervals may not be detected until there
**	is an attempt to parse the data value, this method should
**	only be considered accurate after calling a getXXX() method.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	booolean	True if known to be interval, False otherwise.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
*/

public boolean
isInterval()
{
    return( ! isNull()  &&  interval );
} // isInterval


/*
** Name: isTruncated
**
** Description:
**	Returns an indication that the data value was truncated.
**
**	Note, we don't want to indicate truncation for intervals,
**	but the interval indicator is not valid until after a
**	call to a getXXX() method has been made.  Also, intervals
**	may be longer than timestamps, so the truncated length
**	may actually be longer than the expected length.
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
    return( ! isNull()  &&  ! interval  && 
	    value.length() != SqlDates.TS_FMT.length() );
} // isTruncated


/*
** Name: getDataSize
**
** Description:
**	Returns the expected size of an untruncated data value.
**	For datatypes whose size is unknown or varies, -1 is returned.
**	The returned value is only valid for truncated data values
**	(isTruncated() returns True).
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int    Expected size of data value.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
*/

public int
getDataSize()
{
    return( SqlDates.TS_FMT.length() );
} // getDataSize


/*
** Name: getTruncSize
**
** Description:
**	Returns the actual size of a truncated data value.  For 
**	datatypes whose size is unknown or varies, -1 is returned.
**	The returned value is only valid for truncated data values
**	(isTruncated() returns True).
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int    Truncated size of data value.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
*/

public int
getTruncSize()
{
    return( value.length() );
} // getTruncSize



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
**      27-Oct-10 (rajus01) SIR 124588, SD issue 147074
**          Added ability to store blank date back in the DBMS.
*/

public void 
setString( String value ) 
    throws SQLException
{
    if ( value == null )
    {
	if( empty_date != null )
	    super.setNull();
	else
	{
	    setNotNull();
	    this.value = "";
	    interval = false;
	}	
    }
    else  if ( value.length() == 0  || value.equals( empty_date ) )
    {
        /*
        ** Zero length strings are permitted as a way
        ** of assigning an Ingres empty date value.
        */
        setNotNull();
        this.value = "";
        interval = false;
    }
    else
    {
	/*
	** Input should be a valid timestamp, date, or time
	** literal.  Simply try to convert each format until
	** a valid value is found.
	**
	** Note that Ingres intervals are not supported, but
	** could be by replacing the explict exception with an
	** assignment of input to the data value and leaving
	** validation of the input to the DBMS.
	*/
	Timestamp ts;
	
	try { ts = Timestamp.valueOf( value ); }
	catch( Exception ex_ts )
	{
	    Date date;
	    
	    try { date = Date.valueOf( value ); }
	    catch( Exception ex_d )
	    {
		Time time;
		
		try { time = Time.valueOf( value ); }
		catch( Exception ex_t )
		{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }
		
		setTime( time, null );
		return;
	    }
	    
	    setDate( date, null );
	    return;
	}
	
	setTimestamp( ts, null );
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
**	    Use date/time formatter instance.
**      13-Oct-10 (rajus01) SIR 124588, SD issue 147074
**          Added ability to store blank date back in the DBMS.
*/

public void 
setDate( Date value, TimeZone tz ) 
    throws SQLException
{
    if ( value == null )
    {
	if ( empty_date != null )
	    super.setNull();
	else
	{
	    setNotNull();
	    interval = false;
	    this.value = "";
	}
    }
    else
    {
	/*
	** The date is stored in GMT such as to have a 0 time for
	** the local TZ.  Ingres dates are not associated with a
	** TZ.  Gateways add a 0 time to dates which is assumed to
	** be in the client TZ.  Timezones can be applied in both
	** cases to get the actual date in the desired TZ.  Other-
	** wise, the date in the local TZ is used.
	*/

        java.util.Date dt = null;

	setNotNull();
	interval = false;

	if( empty_date == null )
	    this.value = ( tz != null ) ? dates.formatDate( value, tz )
				    : dates.formatDate( value, false );
	else
	{

	    if( empty_date.length() == 0 ||
                empty_date.length() == SqlDates.T_FMT.length() )
	    {
		if( tz == null )
		    dt = SqlDates.getEpochDate();
		else
	        {
		    String str = SqlDates.D_EPOCH;
		    dt = dates.parseDate( str, tz );
	        }
	    }
	    else if( empty_date.length() == SqlDates.D_FMT.length() )
	    {
		if( tz == null )
		    dt = dates.parseDate( empty_date, false );
		else
		    dt = dates.parseDate( empty_date, tz);
	    }
	    else if( empty_date.length() == SqlDates.TS_FMT.length() )
	    {
		String str = dates.formatDate(
                    dates.parseTimestamp( empty_date, false ), false );
		dt =  dates.parseDate( str, false );
	    }

	    if(  dt != null && dt.getTime() == value.getTime() )
		this.value = "";
	    else
		this.value = ( tz != null ) ? dates.formatDate( value, tz )
				    : dates.formatDate( value, false );
	}
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
**	19-Jun-06 (gordy)
**	    Use osql_dates to determine when to apply external TZ.
**	24-Dec-08 (gordy)
**	    Use date/time formatter instance.
**      27-Oct-10 (rajus01) SIR 124588, SD issue 147074
**	    Added ability to store blank date back in the DBMS only
**	    when the replacement value is null. Any other magic values
**	    will not be mapped back to empty date value.
*/

public void 
setTime( Time value, TimeZone tz ) 
    throws SQLException
{
    if ( value == null )
    {
	if( empty_date != null )
	    super.setNull();
	else
	{
	    /* Map null value to blank date */
	    setNotNull();
	    this.value = "";
	    interval = false;
	}
    }
    else
    {
	/*
	** The time is stored in GMT.  Timezones are not applied to
	** Ingres times since they are also stored in GMT.  OpenSQL
	** times are assumed to be in the client TZ, so timezones
	** can be applied to store values for specific timezones.
	*/
	if ( osql_dates  &&  tz != null )
	{
	    /*
	    ** First retrieve the time in the desired TZ.
	    */
	    String str = dates.formatTime( value, tz );

	    /*
	    ** The local TZ will be applied, either by the driver or
	    ** the gateway, during subsequent processing.  We use the
	    ** local TZ to save the desired value so as to cancel the 
	    ** future application of the local TZ.
	    */
	    value = dates.parseTime( str, false );
	}
	
	/*
	** Produce the correct time value for the current connection.
	**
	** Ingres only partially supports time only values and adds
	** the current date to such values.  JDBC specifies that the
	** date portion for time values should be set to the date
	** epoch 1970-01-01.  When the current date has a different
	** daylight savings offset than the epoch, a one hour offset
	** can occur because of the different GMT offsets applied by
	** Java and Ingres.  Due to these problems, format the time
	** as a timestamp to ensure consistent processing.  Note that
	** formatTimestamp() takes a java.util.Date parameter of which
	** java.sql.Time is a sub-class.
	*/
	setNotNull();
 	this.value = dates.formatTimestamp( value, use_gmt );
	interval = false;
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
**	19-Jun-06 (gordy)
**	    Use osql_dates to determine when to apply external TZ.
**	24-Dec-08 (gordy)
**	    Use date/time formatter instance.
**      13-Oct-10 (rajus01) SIR 124588, SD issue 147074
**          Added ability to store blank date back in the DBMS.
*/

public void 
setTimestamp( Timestamp value, TimeZone tz ) 
    throws SQLException
{
    if ( value == null )
    {
	if ( empty_date != null )
	    super.setNull();
	else
	{
	    setNotNull();
	    interval = false;
	    this.value = "";
	}
    }
    else
    {
	/*
	** The timestamp is stored in GMT.  Timezones are not applied to
	** Ingres timestamps since they are also stored in GMT.  OpenSQL
	** timestamps are assumed to be in the client TZ, so timezones
	** can be applied to store values for specific timezones.
	*/

	Timestamp ts = null;

	if ( osql_dates  &&  tz != null )
	{
	    /*
	    ** First retrieve the timestamp for the desired TZ.
	    */
	    String str = dates.formatTimestamp( value, tz );
	        
	    /*
	    ** The local TZ will be applied, either by the driver or
	    ** the gateway, during subsequent processing.  We use the
	    ** local TZ to save the desired value to cancel the future
	    ** application.
	    */
	    value = dates.parseTimestamp( str, false );
	}
	
	setNotNull();
	interval = false;

	if( empty_date == null )
	    this.value = dates.formatTimestamp( value, use_gmt );
	else
	{
	    if( empty_date.length() == 0 )
	    {
		if( tz == null )
		    ts = SqlDates.getEpochTimestamp();
		else
		    ts = dates.parseTimestamp( SqlDates.TS_EPOCH, tz);
	    }
	    else if( empty_date.length() == SqlDates.T_FMT.length() )
	    {
		java.util.Date dt;

		if( tz == null )
		    dt = dates.parseTime( empty_date, false);
		else
		    dt = dates.parseTime( empty_date, tz);
		ts = new Timestamp( dt.getTime() );
	    }
	    else if( empty_date.length() == SqlDates.D_FMT.length() )
	    {
		java.util.Date dt;

		if( tz == null )
		    dt = dates.parseDate( empty_date, false );
		else
		    dt = dates.parseDate( empty_date, tz);

		ts = new Timestamp( dt.getTime() );
	    }
	    else if( empty_date.length() == SqlDates.TS_FMT.length() )
	    {
		if( tz == null )
		    ts = dates.parseTimestamp( empty_date, false );
		else
		    ts = dates.parseTimestamp( empty_date, tz);
	    }

	    if(  ts != null && ts.getTime() == value.getTime() )
		this.value="";
	    else
		this.value = dates.formatTimestamp( value, use_gmt );
	}
    }
    
    return;
} // setTimestamp


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
**	12-Sep-07 (gordy)
**	    Return replacement string for empty dates.
**	24-Dec-08 (gordy)
**	    Use date/time formatter instance.
*/

public String 
getString() 
    throws SQLException
{
    String str;
    
    /*
    ** Ingres dates are overloaded with 'empty' date,
    ** date only, timestamp and interval values.  The
    ** raw data string is returned for empty dates and
    ** intervals.  Date only values and timestamps are
    ** parsed/formatted to validate and set timezone.  
    ** Intervals will cause an exception if an attempt 
    ** is made to parse the value or will be detected
    ** by a mis-match in expected string lengths.
    */
    try
    {
	if ( value.length() == 0 )				// Empty date
	{
	    /*
	    ** Return the empty date string.
	    */
	    str = empty_date;
	}
	else  if ( value.length() == SqlDates.D_FMT.length() )	// Date only
	{
	    /*
	    ** Do conversion to check for valid format (in
	    ** case this is an interval).  Ingres dates are
	    ** indepedent of timezone, so use local TZ.
	    */
	    Date dt = dates.parseDate( value, false );
	    str = dates.formatDate( dt, false );
	}
	else  if ( value.length() == SqlDates.TS_FMT.length() )	// Timestamp
	{
	    /*
	    ** Convert to GMT using TZ for current connection
	    ** and then to local time using local TZ.
	    */
	    Timestamp ts = dates.parseTimestamp( value, use_gmt );
	    str = dates.formatTimestamp( ts, false );
	}
	else							// Interval
	{
	    /*
	    ** Return the interval string and produce a warning.
	    */
	    interval = true;
	    str = value;
	}
    }
    catch( SQLException ignore )
    {
	/*
	** Any parsing error is assumed to be caused by an interval.
	*/
	interval = true;
	str = value;
    }
    
    return( str );
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
**	12-Sep-03 (gordy)
**	    Created.
**	19-Jun-06 (gordy)
**	    Use osql_dates to determine when to apply external TZ.
**	12-Sep-07 (gordy)
**	    Return replacement value for empty dates.
**	24-Dec-08 (gordy)
**	    Use date/time formatter instance.
*/

public Date 
getDate( TimeZone tz ) 
    throws SQLException
{ 
    String str;

    /*
    ** Ingres dates are overloaded with 'empty' date,
    ** date only, timestamp and interval values.  The
    ** first three types are handled explicitly below.
    ** Intervals will either cause an exception while
    ** attempting to parse the value or as the default
    ** action for an unrecognized format.
    */
    try
    {
	if ( value.length() == 0 )  				// Empty date
	{
	    /*
	    ** Since JDBC does not have any corresponding 
	    ** empty date concept, we use the JDBC date 
	    ** epoch or some replacement value.
	    */
	    if ( empty_date.length() == 0  ||
	         empty_date.length() == SqlDates.T_FMT.length() )
	    {
		/*
		** Local tz date epoch can be handled efficiently,
		** otherwise, epoch must be adjusted by requested tz.
		*/
		if ( tz == null )  return( SqlDates.getEpochDate() );
		str = SqlDates.D_EPOCH;
	    }
	    else  if ( empty_date.length() == SqlDates.D_FMT.length() )
		str = empty_date;
	    else  if ( empty_date.length() == SqlDates.TS_FMT.length() )
	    {
		/*
		** Remove the time component but retain correct date:
		**
		** 1.  Convert to Timestamp using local TZ.
		** 2.  Format as date only using local TZ to get local date.  
		*/
		str = dates.formatDate( 
			dates.parseTimestamp( empty_date, false ), false );
	    }
	    else
		throw SqlExFactory.get( ERR_GC401B_INVALID_DATE );

	    /*
	    ** Generate date for local/requested timezone.
	    */
	    return( (tz == null) ? dates.parseDate( str, false )
				 : dates.parseDate( str, tz ) );
	}
	else  if ( value.length() == SqlDates.D_FMT.length() )	// Date only
	{
	    /*
	    ** The date is stored in GMT such as to have a 0 time for
	    ** the target TZ (requested/local).
	    */
	    return( (tz == null ) ? dates.parseDate( value, false )
	    			  : dates.parseDate( value, tz ) );
	}
	else  if ( value.length() == SqlDates.TS_FMT.length() )	// Timestamp
	{
	    /*
	    ** Remove the time component but retain correct date:
	    **
	    ** 1.  Convert to GMT Timestamp using TZ for current connection.
	    ** 2.  Format as date only using local TZ to get local date.  
	    ** 3.  Generate Date value using requested/local timezone.
	    */
	    str = dates.formatDate( dates.parseTimestamp( value, use_gmt ), 
				    false );
	    return( (osql_dates  &&  tz != null ) 
				? dates.parseDate( str, tz ) 
				: dates.parseDate( str, false ) );
	}
	else							// Interval
	{
	    /*
	    ** Can't support intervals with Date objects.
	    */
	    throw SqlExFactory.get( ERR_GC401B_INVALID_DATE );
	}
    }
    catch( SQLException ex )
    {
	/*
	** Any parsing error is assumed to be caused by an interval.
	*/
	interval = true;
	throw ex;
    }
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
**	19-Jun-06 (gordy)
**	    Use osql_dates to determine when to apply external TZ.
**	12-Sep-07 (gordy)
**	    Return replacement value for empty dates.
**	24-Dec-08 (gordy)
**	    Use date/time formatter instance.
*/

public Time 
getTime( TimeZone tz ) 
    throws SQLException
{ 
    String str;

    /*
    ** Ingres dates are overloaded with 'empty' date,
    ** date only, timestamp and interval values.  The
    ** first three types are handled explicitly below.
    ** Intervals will either cause an exception while
    ** attempting to parse the value or as the default
    ** action for an unrecognized format.
    */
    try
    {
	if ( value.length() == 0 )				// Empty date
	{
	    /*
	    ** Since JDBC does not have any corresponding 
	    ** empty date concept, we use the JDBC time 
	    ** epoch or some replacement value.
	    */
	    if ( empty_date.length() == 0  ||
		 empty_date.length() == SqlDates.D_FMT.length() )
	    {
		/*
		** Local tz time epoch can be handled efficiently,
		** otherwise, epoch must be adjusted by requested tz.
		*/
		if ( tz == null )  return( SqlDates.getEpochTime() );
		str = SqlDates.T_EPOCH;
	    }
	    else  if ( empty_date.length() == SqlDates.T_FMT.length() )
		str = empty_date;
	    else  if ( empty_date.length() == SqlDates.TS_FMT.length() )
	    {
		/*
		** Remove the date component but retain correct time:
		**
		** 1.  Convert to GMT timestamp using TZ for current connection.
		** 2.  Re-format as time only using local TZ to get local time.
		*/
		str = dates.formatTime( 
			dates.parseTimestamp( empty_date, false ), false );
	    }
	    else
	    {
		/*
		** Can't support intervals with Time objects.
		*/
		throw SqlExFactory.get( ERR_GC401B_INVALID_DATE );
	    }

	    /*
	    ** Generate time for local/requested timezone.
	    */
	    return( (tz == null) ? dates.parseTime( str, false )
				 : dates.parseTime( str, tz ) );
	}
	else  if ( value.length() == SqlDates.D_FMT.length() )	// Date only
	{
	    /*
	    ** There is no time component, so return epoch time
	    ** value in local/requested timezone.
	    */
	    return( (tz == null) 
		    ? SqlDates.getEpochTime()
	    	    : dates.parseTime( SqlDates.T_EPOCH, tz ) );
	}
	else  if ( value.length() == SqlDates.TS_FMT.length() )	// Timestamp
	{
	    /*
	    ** Remove the date component but retain correct time:
	    **
	    ** 1.  Convert to GMT timestamp using TZ for current connection.
	    ** 2.  Re-format as time only using local TZ to get local time.
	    ** 3.  Generate Time value using requested/local TZ.
	    */
	    str = dates.formatTime( dates.parseTimestamp( value, use_gmt ), 
				    false );

	    return( (osql_dates  &&  tz != null) 
				? dates.parseTime( str, tz ) 
				: dates.parseTime( str, false ) );
	}
	else							// Interval
	{
	    /*
	    ** Can't support intervals with Time objects.
	    */
	    throw SqlExFactory.get( ERR_GC401B_INVALID_DATE );
	}
    }
    catch( SQLException ex )
    {
	/*
	** Any parsing error is assumed to be caused by an interval.
	*/
	interval = true;
	throw ex;
    }
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
**	tz		Relative timezone, NULL for local.
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
**	19-Jun-06 (gordy)
**	    Use osql_dates to determine when to apply external TZ.
**	12-Sep-07 (gordy)
**	    Return replacement value for empty dates.
**	24-Dec-08 (gordy)
**	    Use date/time formatter instance.
*/

public Timestamp 
getTimestamp( TimeZone tz ) 
    throws SQLException
{
    String str;

    /*
    ** Ingres dates are overloaded with 'empty' date,
    ** date only, timestamp and interval values.  The
    ** first three types are handled explicitly below.
    ** Intervals will either cause an exception while
    ** attempting to parse the value or as the default
    ** action for an unrecognized format.
    */
    try
    {
	if ( value.length() == 0 )				// Empty date
	{
	    if ( empty_date.length() == 0 )
	    {
		/*
		** Local tz timestamp epoch can be handled efficiently,
		** otherwise, epoch must be adjusted by requested tz.
		*/
		if ( tz == null )  return( SqlDates.getEpochTimestamp() );
		str = SqlDates.TS_EPOCH;
	    }
	    else  if ( empty_date.length() == SqlDates.T_FMT.length() )
	    {
		/*
		** There is no date component, so convert to timestamp with
		** a 0 date component for the requested/local timezone.
		*/
		java.util.Date date = (tz == null) 
				      ? dates.parseTime( empty_date, false )
	    			      : dates.parseTime( empty_date, tz );
		return( new Timestamp( date.getTime() ) );
	    }
	    else  if ( empty_date.length() == SqlDates.D_FMT.length() )
	    {
		/*
		** There is no time component, so convert to timestamp with
		** a 0 time component for the requested/local timezone.
		*/
		java.util.Date date = (tz == null) 
				      ? dates.parseDate( empty_date, false )
	    			      : dates.parseDate( empty_date, tz );
		return( new Timestamp( date.getTime() ) );
	    }
	    else  if ( empty_date.length() == SqlDates.TS_FMT.length() )
		str = empty_date;
	    else
	    {
		/*
		** Can't support intervals with Timestamp objects.
		*/
		throw SqlExFactory.get( ERR_GC401B_INVALID_DATE );
	    }

	    /*
	    ** Generate timestamp for local/requested timezone.
	    */
	    return( (tz == null) ? dates.parseTimestamp( str, false )
				 : dates.parseTimestamp( str, tz ) );
	}
	else  if ( value.length() == SqlDates.D_FMT.length() )	// Date only
	{
	    /*
	    ** There is no time component, so convert to timestamp with
	    ** a 0 time component for the requested/local timezone.
	    */
	    java.util.Date date = (tz == null) 
				  ? dates.parseDate( value, false )
	    			  : dates.parseDate( value, tz );
	    return( new Timestamp( date.getTime() ) );
	}
	else  if ( value.length() == SqlDates.TS_FMT.length() )	// Timestamp
	{
	    /*
	    ** Convert to GMT timestamp using TZ for current connection.
	    */
	    Timestamp ts = dates.parseTimestamp( value, use_gmt );
	
	    if ( osql_dates  &&  tz != null )
	    {
	        /*
	        ** Effectively, we need to apply time difference
	        ** between local and requested timezones.  First,
	        ** apply local TZ to get local timestamp.  Then
	        ** apply requested TZ to get desired GMT value.
	        */
	        ts = dates.parseTimestamp( dates.formatTimestamp( ts, false ),
					   tz );
	    }
	
	    return( ts );
	}
	else	   						// Interval
	{
	    /*
	    ** Can't support intervals with Timestamp objects.
	    */
	    throw SqlExFactory.get( ERR_GC401B_INVALID_DATE );
	}
    }
    catch( SQLException ex )
    {
	/*
	** Any parsing error is assumed to be caused by an interval.
	*/
	interval = true;
	throw ex;
    }
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
**	12-Sep-03 (gordy)
**	    Created.
*/

public Object 
getObject() 
    throws SQLException
{
    return( getTimestamp( null ) );
} // getObject

/*
** Name: setNull
**
** Description:
**	Empty date replacement value null needs to stored back as blank date.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**      None.
**
** History:
**	29-Oct-10 (rajus01) SIR 124588, SD issue 147074
**	    Created.
*/
public void
setNull()
{
    if( empty_date == null )
    {
	setNotNull();
	this.value = "";
	interval = false;
    }
    else
	super.setNull();
}

} // class IngresDate
