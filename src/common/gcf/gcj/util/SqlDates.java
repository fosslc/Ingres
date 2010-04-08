/*
** Copyright (c) 2003 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.util;

/*
** Name: SqlDates.java
**
** Description:
**	Defines class which provides constants and methods for
**	processing SQL date/time values.
**
**  Classes:
**
**	SqlDates
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	19-Jun-06 (gordy)
**	    ANSI Date/Time data type support
**	22-Sep-06 (gordy)
**	    Added methods for handling ANSI time zone offsets.
**      17-Nov-08 (rajus01) SIR 121238
**          Replaced SqlEx references with SQLException or SqlExFactory
**          depending upon the usage of it. SqlEx becomes obsolete to support
**          JDBC 4.0 SQLException hierarchy.
*/

import	java.text.DateFormat;
import	java.text.SimpleDateFormat;
import	java.util.Hashtable;
import	java.util.Vector;
import	java.util.TimeZone;
import	java.util.SimpleTimeZone;
import	java.sql.Date;
import	java.sql.Time;
import	java.sql.Timestamp;
import	java.sql.SQLException;


/*
** Name: SqlDates
**
** Description:
**	Utility class which provides constants and methods for
**	date/time processing.
**
**	Date/time formatters must be synchronized on each use.  If
**	concurrent access overhead is not an issue, the default
**	instance can be used.  Otherwise, use a unique instance
**	in each execution context.
**
**  Constants:
**
**	D_EPOCH			Date epoch value.
**	T_EPOCH			Time epoch value.
**	TS_EPOCH		Timestamp epoch value.
**	D_FMT			Date format template.
**	T_FMT			Time format template.
**	TS_FMT			Timestamp format template.
**	D_LIT_FMT		Date literal format template.
**	T_LIT_FMT		Time literal format template.
**	TS_LIT_FMT		Timestamp literal format template.
**	TZ_FMT			Timezone standard format.
**
**  Public Methods:
**
**	getDefaultInstance	Returns default date/time formatters.
**	getInstance		Returns unique date/time formatters.
**	getEpochDate		Returns epoch Date value.
**	getEpochTime		Returns epoch Time value.
**	getEpochTimestamp	Returns epoch Timestamp value.
**
**	getTZ			Returns timezone for UTC offset.
**	parseOffset		Parse Timezone offset.
**	formatOffset		Format Timezone Offset.
**	formatTZ		UTC offset of TZ in standard format.
**	parseFrac		Parse fractional seconds.
**	formatFrac		Format fractional seconds.
**	parseTime		Parse SQL time values.
**	formatTime		Format SQL time values.
**	parseDate		Parse SQL date values.
**	formatDate		Format SQL date values.
**	parseTimestamp		Parse SQL timestamp values.
**	formatTimestamp		Format SQL timestamp values.
**
**  Private Data:
**
**	tz_gmt			GMT Timezone.
**	tz_lcl			Local Timezone.
**	df_ts_lit		Date formatter for Timestamp literals.
**	df_d_lit		Date formatter for Date literals.
**	df_t_lit		Date formatter for Time literals.
**	epochDate		Epoch Date value.
**	epochTime		Epoch Time value.
**	epochTS			Epoch Timestamp value.
**	tzCache			Timezone cache.
**	tzoCache		Timezone offset cache.
**
**	df_date			Formatter for Date values.
**	df_time			Formatter for Time values.
**	df_stamp		Formatter for Timestamp values.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	19-Jun-06 (gordy)
**	    ANSI Date/Time data type support: TZ_FMT, tzOffset(),
**	    parseFrac(), and formatFrac().
**	22-Sep-06 (gordy)
**	    Added parseOffset(), formatOffset(), and getTZ(). 
**	    Rename tzOffset() to formatTZ().  Cache timezone 
**	    offsets: added tzoCache, tzCache.
**	24-Dec-08 (gordy)
**	    Date/time formatters converted to instance values.
**	    Added default instance, getDefaultInstance(),
**	    getInstance(), and getLiteralInstance().  Parsing 
**	    and formattng routines converted to instance methods.  
**	    Removed literal formatters and associated methods,
**	    replaced with a literal instance.
*/

public class
SqlDates
    implements GcfErr
{

    /*
    ** Date/Time constants for EPOCH values, JDBC literals, and
    ** Ingres literals.  
    **
    ** Note: TZ_FMT is provided to show the standard TZ format
    ** but isn't an actual usable format.
    */
    public static final String	D_EPOCH		= "1970-01-01";
    public static final String	T_EPOCH		= "00:00:00";
    public static final String	TS_EPOCH	= D_EPOCH + " " + T_EPOCH;
    public static final String	D_FMT		= "yyyy-MM-dd";
    public static final String	T_FMT		= "HH:mm:ss";
    public static final String	TS_FMT		= D_FMT + " " + T_FMT;
    public static final String	D_LIT_FMT	= "yyyy_MM_dd";
    public static final String	T_LIT_FMT	= "HH:mm:ss";
    public static final String	TS_LIT_FMT	= D_LIT_FMT + " " + T_LIT_FMT;
    public static final String	TZ_FMT		= "+HH:mm";

    /*
    ** Common timezone and date/time values.  There are two fixed 
    ** timezones: GMT and the local timezone.  
    */
    private static TimeZone	tz_gmt = TimeZone.getTimeZone( "GMT" );
    private static TimeZone	tz_lcl = TimeZone.getDefault();
    private static Date		epochDate = null;
    private static Time		epochTime = null;
    private static Timestamp	epochTS = null;
    private static Hashtable	tzCache = new Hashtable();
    private static Vector	tzoCache = new Vector();


    /*
    ** Default date/time formatters instance.
    */
    private static SqlDates	dflt = new SqlDates( false );


    /*
    ** Date/time/timestamp formatters required by parsing and formatting.
    ** Use of any formatter requires synchronization on the formatters 
    ** themselves.
    */
    private DateFormat		df_date;
    private DateFormat		df_time;
    private DateFormat		df_stamp;


/*
** Name: SqlDates
**
** Description:
**	Constructor: private instantiation restricts access
**	to static factory methods.
**
**	Either standard or literal date/time formatters may
**	be selected.
**
** Input:
**	literal		Use literal date/time formatters?
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
**	24-Dec-08 (gordy)
**	    Allocate date/time formatters.
*/

private
SqlDates( boolean literal )
{
    if ( literal )
    {
	df_date = new SimpleDateFormat( D_LIT_FMT );
	df_time = new SimpleDateFormat( T_LIT_FMT );
	df_stamp = new SimpleDateFormat( TS_LIT_FMT );
    }
    else
    {
	df_date = new SimpleDateFormat( D_FMT );
	df_time = new SimpleDateFormat( T_FMT );
	df_stamp = new SimpleDateFormat( TS_FMT );
    }
} // SqlDates


/*
** Name: getDefaultInstance
**
** Description:
**	Returns a static instance of the date/time formatters.
**	This instance should be used when concurrent access is
**	not a concern.
**
**	The date/time formatters are still synchronized to
**	ensure that there will be no concurrancey problems.
**	If concurrancy conflicts are unlikely and saving
**	a unique instance is awkward, using the default
**	instance will reduce the allocation overhead.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	SqlDates	Static instance.
**
** History:
**	24-Dec-08 (gordy)
**	    Created.
*/

public static SqlDates
getDefaultInstance()
{
    return( dflt );
} // getDefaultInstance


/*
** Name: getInstance
**
** Description:
**	Return a unique instance of the date/time formatters.
**	Each execution context should use a unique instance
**	to avoid synchronization conflicts.  The formatters
**	provide standard date/time formats.
**
**	The date/time formatters are still synchronized to
**	ensure that there will be no concurrency problems.
**	Unique instances provide the ability to avoid
**	contention for formatters, but there will be still
**	be some syncronization overhead.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	SqlDates	Unique instance.
**
** History:
**	24-Dec-08 (gordy)
**	    Created.
*/

public static SqlDates
getInstance()
{
    return( new SqlDates( false ) );
} // getInstance


/*
** Name: getLiteralInstance
**
** Description:
**	Return a unique instance of the date/time formatters.
**	Each execution context should use a unique instance
**	to avoid synchronization conflicts.  The formatters
**	provide literal date/time formats.
**
**	The date/time formatters are still synchronized to
**	ensure that there will be no concurrency problems.
**	Unique instances provide the ability to avoid
**	contention for formatters, but there will be still
**	be some syncronization overhead.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	SqlDates	Unique instance.
**
** History:
**	24-Dec-08 (gordy)
**	    Created.
*/

public static SqlDates
getLiteralInstance()
{
    return( new SqlDates( true ) );
}


/*
** Name: getEpochDate
**
** Description:
**	Returns epoch Date value relative to local timezone.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Date	Epoch date value.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Use default formatters.
*/

public static Date
getEpochDate()
    throws SQLException
{
    if ( epochDate == null )  epochDate = dflt.parseDate( D_EPOCH, tz_lcl );
    return( epochDate );
} // getEpochDate


/*
** Name: getEpochTime
**
** Description:
**	Returns epoch Time value relative to local timezone.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Time	Epoch Time value.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Use default formatters.
*/

public static Time
getEpochTime()
    throws SQLException
{
    if ( epochTime == null )  epochTime = dflt.parseTime( T_EPOCH, tz_lcl );
    return( epochTime );
} // getEpochTime


/*
** Name: getEpochTimestamp
**
** Description:
**	Returns epoch Timestamp value relative to local timezone.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Timestamp	Epoch Timestamp value.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Use default formatters.
*/

public static Timestamp
getEpochTimestamp()
    throws SQLException
{
    if ( epochTS == null )  epochTS = dflt.parseTimestamp( TS_EPOCH, tz_lcl );
    return( epochTS );
} // getEpochTimestamp


/*
** Name: getTZ
**
** Description:
**	Returns a timezone for a given TZ offset.
**
** Input:
**	offset	Timezone offset in minutes.
**
** Output:
**	None.
**
** Returns:
**	TimeZone	Timezone.
**
** History:
**	22-Sep-06 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Converted to instance method.
*/

public TimeZone
getTZ( int offset )
    throws SQLException
{
    return( getTZ( formatOffset( offset ) ) );
} // getTZ


/*
** Name: getTZ
**
** Description:
**	Parses standard TZ offset format "+HH:mm" and returns
**	a timezone with the same TZ offset.
**
** Input:
**	tzOffset	TZ offset.
**
** Output:
**	None.
**
** Returns:
**	TimeZone	Timezone.
**
** History:
**	22-Sep-06 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Converted to instance method.
*/

public TimeZone
getTZ( String tzOffset )
    throws SQLException
{
    /*
    ** Only a small number timezones are expected to be active.
    ** Timezones and offsets are cached for quick access.
    */
    TimeZone tz = (TimeZone)tzCache.get( tzOffset );

    if ( tz == null )
    {
	/*
	** Construct timezone for offset
	*/
	int offset = parseOffset( tzOffset );
	tz = new SimpleTimeZone( offset * 60000, "GMT" + tzOffset );
	tzCache.put( tzOffset, tz );
    }

    return( tz );
} // getTZ


/*
** Name: parseOffset
**
** Description:
**	Parses standard timezone offset "+HH:mm" and returns
**	offset in minutes.
**
** Input:
**	tzOffset	Standard timezone offset.
**
** Output:
**	None.
**
** Returns:
**	int		Offset in minutes.
**
** History:
**	22-Sep-06 (gordy
**	    Created.
**	24-Dec-08 (gordy)
**	    Converted to instance method.
*/

public int
parseOffset( String tzOffset )
    throws SQLException
{
    /*
    ** Validate offset format.
    */
    if (
	 tzOffset.length() != TZ_FMT.length()  ||
	 (tzOffset.charAt(0) != '+'  &&  tzOffset.charAt(0) != '-')  ||
	 ! Character.isDigit( tzOffset.charAt(1) )  ||
	 ! Character.isDigit( tzOffset.charAt(2) )  ||
	 tzOffset.charAt(3) != ':'  ||
	 ! Character.isDigit( tzOffset.charAt(4) )  ||
	 ! Character.isDigit( tzOffset.charAt(5) )
       )
	throw SqlExFactory.get( ERR_GC401B_INVALID_DATE );

    int hours = Character.digit( tzOffset.charAt(1), 10 ) * 10 +
		Character.digit( tzOffset.charAt(2), 10 );
    int minutes = Character.digit( tzOffset.charAt(4), 10 ) * 10 +
		  Character.digit( tzOffset.charAt(5), 10 );
    int offset = hours * 60 + minutes;
    if ( tzOffset.charAt(0) == '-' )  offset = -offset;

    return( offset );
} // parseOffset


/*
** Name: formatOffset
**
** Description:
**	Formats a timesone offset into standard format: "+HH:mm".
**
** Input:
**	offset		Timezone offset in minutes.
**
** Output:
**	None.
**
** Returns:
**	String		Formatted offset.
**
** History:
**	22-Sep-06 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Converted to instance method.
*/

private static char digits[] = {'0','1','2','3','4','5','6','7','8','9'};

public String
formatOffset( int offset )
{
    String tzOffset = null;

    /*
    ** Since only a small number of timezones are expected
    ** to be active, a simple vector is used to map offsets.
    ** The vector is not synchronized because it is only
    ** extended and duplicate entries will only cause an
    ** insignificant degredation in performance.
    */
    for( int i = 0; i < tzoCache.size(); i++ )
    {
	IdMap tzo = (IdMap)tzoCache.get(i);

	if ( tzo.equals( offset ) )
	{
	    tzOffset = tzo.toString();
	    break;
	}
    }

    if ( tzOffset == null )
    {	
	boolean neg = (offset < 0);
	if ( neg )  offset = -offset;
	int hour = offset / 60;
	int minute = offset % 60;

	char format[] = new char[ 6 ];
	format[ 0 ] = neg ? '-' : '+';
	format[ 1 ] = digits[ (hour / 10) % 10 ];
	format[ 2 ] = digits[ hour % 10 ];
	format[ 3 ] = ':';
	format[ 4 ] = digits[ (minute / 10) % 10 ];
	format[ 5 ] = digits[ minute % 10 ];

	if ( neg )  offset = -offset;	// Restore original offset
	tzOffset = new String( format );
	tzoCache.add( new IdMap( offset, tzOffset ) );
    }

    return( tzOffset );
} // formatOffset


/*
** Name: formatTZ
**
** Description:
**	Returns the offset from UTC for a date/time value 
**	in the local default timezone.  Standard TZ offset
**	format is "+HH:mm".
**
** Input:
**	value		Date/time value.
**
** Output:
**	None.
**
** Returns:
**	String		TZ offset.
**
** History:
**	19-Jun-06 (gordy)
**	    Created.
**	22-Sep-06 (gordy)
**	    Renamed.
**	24-Dec-08 (gordy)
**	    Converted to instance method.
*/

public String
formatTZ( java.util.Date value )
{
    return( formatTZ( tz_lcl, value.getTime() ) );
} // formatTZ


/*
** Name: formatTZ
**
** Description:
**	Returns the offset from UTC for a date/time value 
**	in a specific timezone.  Standard TZ offset format
**	is "+HH:mm".
**
** Input:
**	tz		Target timezone.
**	value		Date/time value.
**
** Output:
**	None.
**
** Returns:
**	String		UTC offset.
**
** History:
**	19-Jun-06 (gordy)
**	    Created.
**	22-Sep-06 (gordy)
**	    Renamed.  
**	24-Dec-08 (gordy)
**	    Converted to instance method.
*/

public String
formatTZ( TimeZone tz, java.util.Date value )
{
    return( formatTZ( tz, value.getTime() ) );
} // formatTZ


/*
** Name: formatTZ
**
** Description:
**	Returns the offset from UTC for a milli-second time
**	value in the default timezone.  Standard TZ
**	offset format is "+HH:mm".
**
** Input:
**	millis		Milli-second time value.
**
** Output:
**	None.
**
** Returns:
**	String		UTC offset.
**
** History:
**	22-Sep-06 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Converted to instance method.
*/

public String
formatTZ( long millis )
{
    return( formatTZ( tz_lcl, millis ) );
} // formatTZ


/*
** Name: formatTZ
**
** Description:
**	Returns the offset from UTC for a date/time value
**	in a specific timezone.  Standard TZ offset format
**	is "+HH:mm".
**
** Input:
**	tz		Target timezone.
**	millis		Milli-second time value.
**
** Output:
**	None.
**
** Returns:
**	String		UTC ofset.
**
** History:
**	22-Sep-06 (gordy)
**	    Created.  Cache offset strings.
**	24-Dec-08 (gordy)
**	    Converted to instance method.
*/

public String
formatTZ( TimeZone tz, long millis )
{
    return( formatOffset( tz.getOffset( millis ) / 60000 ) );
} // formatTZ


/*
** Name: parseFrac
**
** Description:
**	Parses a Fractional seconds timestamp component
**	in the format ".ddddddddd" and returns the number
**	of nano-seconds.  At a minimum, the decimal point
**	must be present.  Any number of decimal digits 
**	may be present.  Any digits beyond nano-seconds
**	are ignored.
**
** Input:
**	frac	Fractional seconds string.
**
** Output:
**	None.
**
** Returns:
**	int	Nano-seconds
**
** History:
**	19-Jun-06 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Converted to instance method.
*/

private static char zeros[] = {'0','0','0','0','0','0','0','0','0'};

public int
parseFrac( String frac )
    throws SQLException
{
    int nanos = 0;

    /*
    ** Validate minimum format.
    */
    if ( frac.length() < 1  ||  frac.charAt( 0 ) != '.' )
	throw SqlExFactory.get( ERR_GC401B_INVALID_DATE );

    /*
    ** Transform into standard format by truncating extra 
    ** digits or appending trailing zeros as needed.
    */
    StringBuffer sb = new StringBuffer( frac );
    if ( sb.length() > 10 )  sb.setLength( 10 );
    if ( sb.length() < 10 )  sb.append( zeros, 0, 10 - sb.length() );

    /*
    ** Extract number of nano-seconds.
    */
    try { nanos = Integer.parseInt( sb.substring( 1 ) ); }
    catch( NumberFormatException nfe )
    { throw SqlExFactory.get( ERR_GC401B_INVALID_DATE ); }

    return( nanos );
} // parseFrac


/*
** Name: formatFrac
**
** Description:
**	Formats nano-seconds into a fractional seconds
**	timestamp component in the format ".dddddddddd".
**	Trailing zeros are truncated, but at a minimum
**	".0" will be returned.
**
** Input:
**	nanos	Nano-seconds
**
** Output:
**	None.
**
** Returns:
**	String	Fractional seconds string.
**
** History:
**	19-Jun-06 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Converted to instance method.
*/

public String
formatFrac( int nanos )
{
    /*
    ** Handle minimal case.
    */
    if ( nanos == 0 )  return( ".0" );
    if ( nanos < 0 )  nanos = -nanos;		// Shouldn't happen!

    StringBuffer sb = new StringBuffer();
    String	 frac = Integer.toString( nanos );

    /*
    ** Build-up the fractional seconds component.
    ** Prepend zeros as necessary to get correct
    ** magnitude.  
    */
    sb.append( '.' );
    if ( frac.length() < 9 )  sb.append( zeros, 0, 9 - frac.length() );
    sb.append( frac );

    /*
    ** Remove trailing zeros.  There shouldn't be any
    ** excess digits, but skip/truncate just in case.
    ** Note: there is at least 1 non-zero digit since
    ** the 0 case was handled above.
    */
    int length = 10;
    while( sb.charAt( length - 1 ) == '0' )  length--;
    sb.setLength( length );

    return( sb.toString() );
} // formatFrac


/*
** Name: parseTime
**
** Description:
**	Parse a string containing a JDBC time value using GMT or
**	local timezone.
**
** Input:
**	str		Time value.
**	use_gmt		True for GMT, False for local timezone.
**
** Output:
**	None.
**
** Returns:
**	Time		JDBC Time.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Converted to instance method.
*/

public Time
parseTime( String str, boolean use_gmt ) 
    throws SQLException
{ 
    return( parseTime( str, use_gmt ? tz_gmt : tz_lcl ) ); 
}


/*
** Name: parseTime
**
** Description:
**	Parse a string containing a JDBC time value using an 
**	explicit timezone.
**
** Input:
**	str		Time value.
**	tz		Timezone.
**
** Output:
**	None.
**
** Returns:
**	Time		JDBC Time.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Converted to instance method.
*/

public Time
parseTime( String str, TimeZone tz )
    throws SQLException
{
    java.util.Date date;
    
    if ( str.length() != T_FMT.length() )
	throw SqlExFactory.get( ERR_GC401B_INVALID_DATE );

    synchronized( df_time )
    {
	df_time.setTimeZone( tz );
	try { date = df_time.parse( str ); }
	catch( Exception ex )
	{ throw SqlExFactory.get( ERR_GC401B_INVALID_DATE ); }
    }
    
    return( new Time( date.getTime() ) );
} // parseTime


/*
** Name: formatTime
**
** Description:
**	Format a Java date value into a time string using GMT or
**	local timezone.
**
** Input:
**	date		Jave date value.
**	use_gmt		Use GMT or local timezone.
**
** Output:
**	None.
**
** Returns:
**	String		Formatted time.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Converted to instance method.
*/

public String
formatTime( java.util.Date date, boolean use_gmt )
{ 
    return( formatTime( date, use_gmt ? tz_gmt : tz_lcl ) ); 
} // formatTime


/*
** Name: formatTime
**
** Description:
**	Format a Java date value into a time string using an 
**	explicit timezone.
**
** Input:
**	date		Jave date value.
**	tz		Timezone.
**
** Output:
**	None.
**
** Returns:
**	String		Formatted time.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Converted to instance method.
*/

public String
formatTime( java.util.Date date, TimeZone tz )
{
    String str;
    
    synchronized( df_time )
    {
	df_time.setTimeZone( tz );
	str = df_time.format( date );
    }
    
    return( str );
} // formatTime


/*
** Name: parseDate
**
** Description:
**	Parse a string containing a JDBC date value using GMT or
**	local timezone.
**
** Input:
**	str		Date value.
**	use_gmt		True for GMT, Falst for local timezone.
**
** Output:
**	None.
**
** Returns:
**	Date		JDBC Date.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Converted to instance method.
*/

public Date
parseDate( String str, boolean use_gmt ) 
    throws SQLException
{ 
    return( parseDate( str, use_gmt ? tz_gmt : tz_lcl ) ); 
} // parseDate


/*
** Name: parseDate
**
** Description:
**	Parse a string containing a JDBC date value using an 
**	explicit timezone.
**
** Input:
**	str		Date value.
**	tz		Timezone.
**
** Output:
**	None.
**
** Returns:
**	Date		JDBC Date.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Converted to instance method.
*/

public Date
parseDate( String str, TimeZone tz )
    throws SQLException
{
    java.util.Date date;
    
    if ( str.length() != D_FMT.length() )
	throw SqlExFactory.get( ERR_GC401B_INVALID_DATE );

    synchronized( df_date )
    {
	df_date.setTimeZone( tz );
	try { date = df_date.parse( str ); }
	catch( Exception ex )
	{ throw SqlExFactory.get( ERR_GC401B_INVALID_DATE ); }
    }
    
    return( new Date( date.getTime() ) );
} // parseDate


/*
** Name: formatDate
**
** Description:
**	Format a Java date value into a date string using GMT or
**	local timezone.
**
** Input:
**	date		Jave date value.
**	use_gmt		True for GMT, False for local timezone.
**
** Output:
**	None.
**
** Returns:
**	String		Formatted date.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Converted to instance method.
*/

public String
formatDate( java.util.Date date, boolean use_gmt )
{ 
    return( formatDate( date, use_gmt ? tz_gmt : tz_lcl ) ); 
} // formatDate


/*
** Name: formatDate
**
** Description:
**	Format a Java date value into a date string using an 
**	explicit timezone.
**
** Input:
**	date		Jave date value.
**	tz		Timezone.
**
** Output:
**	None.
**
** Returns:
**	String		Formatted date.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Converted to instance method.
*/

public String
formatDate( java.util.Date date, TimeZone tz )
{
    String str;
    
    synchronized( df_date )
    {
	df_date.setTimeZone( tz );
	str = df_date.format( date );
    }
    
    return( str );
} // formatDate


/*
** Name: parseTimestamp
**
** Description:
**	Parse a string containing a JDBC timestamp value using GMT or
**	local timezone.
**
** Input:
**	str		Timestamp value.
**	use_gmt		True for GMT, False for local timezone.
**
** Output:
**	None.
**
** Returns:
**	Timestamp	JDBC Timestamp.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Converted to instance method.
*/

public Timestamp
parseTimestamp( String str, boolean use_gmt ) 
    throws SQLException
{ 
    return( parseTimestamp( str, use_gmt ? tz_gmt : tz_lcl ) ); 
} // parseTimestamp


/*
** Name: parseTimestamp
**
** Description:
**	Parse a string containing a JDBC timestamp value using an 
**	explicit timezone.
**
** Input:
**	str		Timestamp value.
**	tz		Timezone.
**
** Output:
**	None.
**
** Returns:
**	Timestamp	JDBC Timestamp.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Converted to instance method.
*/

public Timestamp
parseTimestamp( String str, TimeZone tz )
    throws SQLException
{
    java.util.Date date;
    
    if ( str.length() != TS_FMT.length() )
	throw SqlExFactory.get( ERR_GC401B_INVALID_DATE );

    synchronized( df_stamp )
    {
	df_stamp.setTimeZone( tz );
	try { date = df_stamp.parse( str ); }
	catch( Exception ex )
	{ throw SqlExFactory.get( ERR_GC401B_INVALID_DATE ); }
    }
    
    return( new Timestamp( date.getTime() ) );
} // parseTimestamp


/*
** Name: formatTimestamp
**
** Description:
**	Format a Java date value into a timestamp string using GMT or
**	local timezone.
**
** Input:
**	date		Jave date value.
**	use_gmt		True for GMT, False for local timezone.
**
** Output:
**	None.
**
** Returns:
**	String		Formatted timestamp.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Converted to instance method.
*/

public String
formatTimestamp( java.util.Date date, boolean use_gmt )
{ 
    return( formatTimestamp( date, use_gmt ? tz_gmt : tz_lcl ) ); 
} // formatTimestamp


/*
** Name: formatTimestamp
**
** Description:
**	Format a Java date value into a timestamp string using an
**	explicit timezone.
**
** Input:
**	date		Jave date value.
**	tz		Timezone.
**
** Output:
**	None.
**
** Returns:
**	String		Formatted timestamp.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	24-Dec-08 (gordy)
**	    Converted to instance method.
*/

public String
formatTimestamp( java.util.Date date, TimeZone tz )
{
    String str;
    
    synchronized( df_stamp )
    {
	df_stamp.setTimeZone( tz );
	str = df_stamp.format( date );
    }
    
    return( str );
} // formatTimestamp


} // class SqlDates
