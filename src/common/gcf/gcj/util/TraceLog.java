/*
** Copyright (c) 1999, 2006 Ingres Corporation All Rights Reserved.
*/

package com.ingres.gcf.util;

/*
** Name: TraceLog.java
**
** Description:
**	Defines a class to manage trace logs.  Supports individual
**	trace logs, key associated trace logs and a default trace
**	log.
**
**	Individual trace logs are created for configuration property
**	sets and initialized accordingly (the configuration key prefix
**	being a part of the configuration property set).
**
**	Trace logs may be explicitly associated with a trace key and a
**	single trace log will be maintained per key.  The key is also 
**	used to build a configuration property set from system properties.  
**
**	In both the preceding cases, the following properties are used:
**
**	    <key>.trace.log		Initial trace log.
**	    <key>.trace.timestamp	Trace timestamps enabled?
**	    <key>.trace.<id>		Trace level for trace id.
**
**	A trace log maintains a set of tracing ID's and an associated
**	tracing level, but level based tracing is not directly supported.
**
**	A single default trace log may also be accessed, but configuration
**	must be done externally as there is no associated property set for
**	initialization.
**
**  Classes
**
**	TraceLog	Class representing a trace log.
**	TraceLevel	Class representing a tracing level.
**
** History:
**	 6-May-99 (gordy)
**	    Created.
**	22-Sep-99 (gordy)
**	    Replaced deprecated PrintStream with PrintWriter.
**	20-Apr-00 (gordy)
**	    Moved to package util.  Reworked as stand-alone class.
**	15-Nov-00 (gordy)
**	    Support trace settings in system properties.
**	 5-Feb-02 (gordy)
**	    Added timestamps.
**	20-Mar-02 (gordy)
**	    The System method getProperty() may throw a security exception
**	    (especially in browser applets).  Ignore as if properties not set.
**	11-Sep-02 (gordy)
**	    Extracted from Tracing to represent trace logs for support of 
**	    multiple logs.  Tracing levels maintained with log but level
**	    based tracing remains in original class.
**	31-Oct-02 (gordy)
**	    Added finalize() method.
**	15-Jul-03 (gordy)
**	    Extended to support configuration property sets.
**	 3-Jul-06 (gordy)
**	    Added new factory method.
*/

import	java.io.PrintWriter;
import	java.io.FileOutputStream;
import	java.io.FileNotFoundException;
import	java.text.DateFormat;
import	java.text.SimpleDateFormat;
import	java.util.Date;
import	java.util.Hashtable;


/*
** Name: TraceLog
**
** Description:
**	Class which represents a trace log and a set of associated
**	tracing ID's and levels.  
**
**  Public Methods:
**
**	getTraceLog	    Factory for TraceLog objects.
**	enabled		    Is trace log enabled.
**	setTraceLog	    Set trace log output file.
**	getTraceLevel	    Get tracing level for trace ID.
**	write		    Write to trace log.
**	hexdump		    Write hexdump to trace log.
**
**  Private Methods:
**
**	timestamp	    Returns timestamp string for current time.
**
**  Private Data:
**
**	TRACE_KEY_PREFIX    Prefix for trace key.
**	TRACE_LOG_KEY	    Key for trace log property.
**	TRACE_TIME_KEY	    Key for timestamp property.
**
**	config		    Configuration property set.
**	trace_log	    Trace output.
**	levels		    Table of trace IDs and levels.
**	trace_time	    Traces include timestamps?
**
**	trace_logs	    Table of trace logs and keys.
**	default_log	    Default trace log.
**	now		    Date used to create timestamps.
**	last_time	    Previous timestamp time.
**	last_ts		    Previous timestamp.
**	df_ts		    Date format for timestamps.
**	hex_buff	    Buffer used by hexdump();
**	hex		    Hex character conversion array.
**
** History:
**	 6-May-99 (gordy)
**	    Created.
**	22-Sep-99 (gordy)
**	    Replaced deprecated PrintStream with PrintWriter.
**	20-Apr-00 (gordy)
**	    Reworked as stand alone class rather than base class.
**	    Added per-instance data/members and static registry
**	    to permit multiple tracing levels and ID's.
**	15-Nov-00 (gordy)
**	    Added KEY_TRACE, KEY_TRACE_LOG, and static initializer
**	    to support trace settings in the system properties.
**	 5-Feb-02 (gordy)
**	    Added timestamp(), KEY_TRACE_TIME, trace_time, df_ts.
**	11-Sep-02 (gordy)
**	    Reworked to represent individual trace logs.  Added static
**	    table for logs.  Log specific items now instance variables.
**	    Added factory method, getTraceLog(), for managing logs.
**	31-Oct-02 (gordy)
**	    Added finalize() method.
**	15-Jul-03 (gordy)
**	    Added support for configuration property sets: TRACE_KEY_PREFIX,
**	    TRACE_LOG_KEY, TRACE_TIME_KEY, factory for getting a trace log
**	    associated with a configuration set and updated constructors.
**	 3-Jul-06 (gordy)
**	    Added getTraceLog() factory method with key and config parameters.
*/

public class
TraceLog
{

    /*
    ** Constants
    */
    private static final String	TRACE_KEY_PREFIX    = "trace";
    private static final String	TRACE_LOG_KEY	    = "log";
    private static final String	TRACE_TIME_KEY	    = "timestamp";
    
    /*
    ** Instance variables.
    */
    private Config		config = null;		    // Properties.
    private PrintWriter		trace_log = null;	    // Trace log.
    private Hashtable		levels = new Hashtable();   // Trace ID's
    private boolean		trace_time = false;	    // Timestamps?

    
    /*
    ** Trace logs and associated trace keys are maintained
    ** in a hash table.  A default log is maintained separatly.
    */
    private static  Hashtable	trace_logs = new Hashtable();
    private static  TraceLog	default_log = null;

    /*
    ** Date objects used by timestamp()
    ** (requires synchronization).
    */
    private static  Date	now = new Date();
    private static  long	last_time = 0;
    private static  String	last_ts = "";
    private static  DateFormat	df_ts = 
				new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS");

    /*
    ** Output buffer and conversion array used by hexdump()
    ** (requires synchronization)
    */
    private static char		hex_buff[] = new char[ (4 * 16) + 4 ];
    private static final char	hex[] = 
    { '0', '1', '2', '3', '4', '5', '6', '7',
      '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };


/*
** Name: getTraceLog
**
** Description:
**	Factory method for allocating TraceLog objects.  Maintains
**	a single default TraceLog object which is not initialized
**	from a configuration property set or system properties.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	TraceLog    Default trace log.
**
** History:
**	11-Sep-02 (gordy)
**	    Created.
**	15-Jul-03 (gordy)
**	    Added support for configuration sets: default trace log 
**	    has empty config set.
*/

public static TraceLog
getTraceLog()
{
    if ( default_log == null )  
	default_log = new TraceLog( new ConfigEmpty() );
    return( default_log );
} // getTraceLog


/*
** Name: getTraceLog
**
** Description:
**	Factory method for allocating TraceLog objects.  Maintains
**	a single TraceLog object for each trace key.  Configuration
**	is enabled via the system properties.  Adds 'trace' as key
**	prefix.
**
** Input:
**	key	    Trace key.
**
** Output:
**	None.
**
** Returns:
**	TraceLog    Trace log for key.
**
** History:
**	11-Sep-02 (gordy)
**	    Created.
**	15-Jul-03 (gordy)
**	    Added support for configuration sets: build config set
**	    from system properties and use trace key as prefix.
*/

public static TraceLog
getTraceLog( String key )
{
    TraceLog log;

    if ( (log = (TraceLog)trace_logs.get( key ) ) == null )
    {
	/*
	** Build configuration property set for accessing
	** system properties using prefix: <key>.trace
	*/
	Config config = new ConfigSys();		// System properties
	config = new ConfigKey( key, config );		// Their key	
	log = new TraceLog( config );
	trace_logs.put( key, log );
    }

    return( log );
} // getTraceLog


/*
** Name: getTraceLog
**
** Description:
**	Factory method for allocating TraceLog objects.  Maintains
**	a single TraceLog object for each trace key.  Configuration
**	is enabled via the configuration set provided.  Adds 'trace' 
**	as key prefix.
**
** Input:
**	key	    Trace key.
**
** Output:
**	None.
**
** Returns:
**	TraceLog    Trace log for key.
**
** History:
**	 3-Jul-06 (gordy)
**	    Created.
*/

public static TraceLog
getTraceLog( String key, Config config )
{
    TraceLog log;

    if ( (log = (TraceLog)trace_logs.get( key ) ) == null )
    {
	log = new TraceLog( config );
	trace_logs.put( key, log );
    }

    return( log );
} // getTraceLog



/*
** Name: getTraceLog
**
** Description:
**	Factory method for allocating TraceLog objects.  Creates a
**	new TraceLog with configuration based on the configuration
**	set provided.  Adds 'trace' as key prefix.
**
** Input:
**	config	    Configuration properties.
**
** Output:
**	None.
**
** Returns:
**	TraceLog    Trace log for key.
**
** History:
**	15-Jul-03 (gordy)
**	    Created.
*/

public static TraceLog
getTraceLog( Config config )
{
    return( new TraceLog( config ) );
} // getTraceLog


/*
** Name: TraceLog
**
** Description:  
**	Class constructor.  Initializes trace log and trace
**	ID's/levels from config properties.  Automatically
**	adds 'trace' as config key prefix.
**
** Input:
**	config	Configuration properties.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	20-Apr-00 (gordy)
**	    Created.
**	15-Nov-00 (gordy)
**	    Check system properties for tracing level.
**	20-Mar-02 (gordy)
**	    Handle security exceptions thrown by getProperty().
**	11-Sep-02 (gordy)
**	    Initialize trace log based on system properties.
**	15-Jul-03 (gordy)
**	    Changed parameter to a configuration set to allow multiple 
**	    configuration sources (including system properties).  
*/

private
TraceLog( Config config )
{
    String value;
    
    this.config = new ConfigKey( TRACE_KEY_PREFIX, config );

    if ( (value = this.config.get( TRACE_LOG_KEY )) != null )  
        setTraceLog( value );

    if ( (value = this.config.get( TRACE_TIME_KEY ) ) != null )
        if ( 
	     value.equalsIgnoreCase( "on" )  || 
	     value.equalsIgnoreCase( "true" )  ||  
	     value.equalsIgnoreCase( "enable" )  ||
	     value.equalsIgnoreCase( "enabled" )
           )
           trace_time = true;

    return;
} // TraceLog


/*
** Name: finalize
**
** Description:
**	Make sure the trace log is closed.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	31-Oct-02 (gordy)
**	    Created.
*/

protected void
finalize()
    throws Throwable
{
    setTraceLog( null );
    super.finalize();
    return;
} // finalize


/*
** Name: enabled
**
** Description:
**	Is trace log enabled?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if trace log enabled.
**
** History:
**	 6-May-99 (gordy)
**	    Created.
**	11-Sep-02 (gordy)
**	    Extracted into trace log class, separate from tracing level.
*/

public boolean
enabled()
{
    return( trace_log != null );
} // enabled


/*
** Name: setTraceLog
**
** Description:
**	Set the trace log.  If the log name is NULL, tracing will 
**	be disabled.
**
** Input:
**	name	    New trace log file name, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	boolean	    False if error opening trace log.
**
** History:
**	 6-May-99 (gordy)
**	    Created.
*/

public boolean
setTraceLog( String name )
{
    boolean success = true;

    if ( trace_log != null )
    {
	trace_log.close();
	trace_log = null;
    }

    if ( name != null )  
	try 
	{ 
	    trace_log = new PrintWriter( new FileOutputStream( name ), true );
	}
	catch( FileNotFoundException ex ) 
	{
	    trace_log = null;
	    success = false; 
	}

    return( success );
} // setTraceLog


/*
** Name: getTraceLevel
**
** Description:
**	Get the tracing level for a trace ID.  The level is determined
**	by any previous setting, a configuration property setting or
**	the default value of 0.  A trace level is set by using the set
**	method of the TraceLevel object returned by this method.
**
** Input:
**	id	    Trace ID.
**
** Output:
**	None.
**
** Returns:
**	int	    Current tracing level.
**
** History:
**	20-Apr-00 (gordy)
**	    Created.
**	11-Sep-02 (gordy)
**	    Check <key>.<id> system property.  Save if not already in table.
**	15-Jul-03 (gordy)
**	    Use configuration set with built in key prefix.
*/

public TraceLevel
getTraceLevel( String id )
{
    TraceLevel	    level;
    Integer	    value;
    
    /*
    ** See if ID has already been processed.
    */
    if ( (level = (TraceLevel)levels.get( id )) == null )
    {
	String  str;
	int	val = 0;

	/*
	** See if ID is defined for trace key.
	*/
	if ( (str = config.get( id )) != null )
	    try { val = Integer.parseInt( str ); }
	    catch( Exception ignore ) {}

	/*
	** Save determined value for future reference.
	*/
	level = new TraceLevel( val );
	levels.put( id, level );
    }

    return( level );
} // getTraceLevel


/*
** Name: write
**
** Description:
**	Write a line to the trace log (if open).  A line terminator 
**	is appended to the output.
**
** Input:
**	str	    String to be written to the trace log.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 6-May-99 (gordy)
**	    Created.
**	 5-Feb-02 (gordy)
**	    Added optional timestamp.
**	11-Sep-02 (gordy)
**	    Use print() methods to reduce string manipulations.
*/

public void
write( String str )
{
    if ( trace_log != null )
    {
	if ( trace_time )  
	{
	    trace_log.print( timestamp() );
	    trace_log.print( ": " );
	}

	trace_log.println( str );
    }

    return;
} // write


/*
** Name: hexdump
**
** Description:
**	Format a hex dump of a byte array (subset) and write to
**	the trace file (if open).
**
** Input:
**	buffer	    Byte array.
**	offset	    Starting byte to be output.
**	length	    Number of bytes to output.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 6-May-99 (gordy)
**	    Created.
**	11-Sep-02 (gordy)
**	    Use println() method to avoid string conversion.
*/

public void
hexdump( byte buffer[], int offset, int length )
{
    if ( trace_log != null )
	synchronized( hex_buff )
	{
	    while( length > 0 )
	    {
		for( int i = 0; i < hex_buff.length; i++ )
		    hex_buff[ i ] = ' ';

		for( int i = 0; length > 0  &&  i < 16; i++ )
		{
		    hex_buff[ i * 3 ] = hex[(buffer[offset] >> 4) & 0x0F];
		    hex_buff[ (i * 3) + 1 ] = hex[buffer[offset] & 0x0F];
		    hex_buff[ (16 * 3) + 4 + i ] = 
			( buffer[ offset ] >= 0x20  &&
			  buffer[ offset ] < 0x7F )
			? (char)buffer[ offset ] : '.';
		    offset++;
		    length--;
		}

		trace_log.println( hex_buff );
	    }
	}
    
    return;
} // hexdump


/*
** Name: timestamp
**
** Description:
**	Returns a timestamp string for the current time.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	    Timestamp string.
**
** History:
**	 5-Feb-02 (gordy)
**	    Created.
**	11-Sep-02 (gordy)
**	    Save and re-use timestamps.
*/

private static String
timestamp()
{
    long time = System.currentTimeMillis();

    if ( time != last_time )
	synchronized( now )
	{
	    now.setTime( (last_time = time) );
	    last_ts = df_ts.format( now );
	}

    return( last_ts );
} // timestamp


/*
** Name: TraceLevel
**
** Description:
**	Class which defines a tracing level maintained by a trace log
**	while providing efficent access by an external tracer.
**
**  Public Methods
**
**	get		Returns current tracing level.
**	set		Set current tracing level.
**
**  Private Data
**
**	level		Current tracing level.
**
** History:
**	11-Sep-02 (gordy)
**	    Created.
*/

public static class
TraceLevel
{

    private int		level;		// Current trace level.


/*
** Name: TraceLevel
**
** Description:
**	Class constructor.  Set the initial tracing level.
**
** Input:
**	level	Tracing level.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	11-Sep-02 (gordy)
**	    Created.
*/

public
TraceLevel( int level )
{
    this.level = level;
} // TraceLevel


/*
** Name: get
**
** Description:
**	Returns the current tracing level.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Tracing level.
**
** History:
**	11-Sep-02 (gordy)
**	    Created.
*/

public int
get()
{
    return( level );
} // get


/*
** Name: set
**
** Description:
**	Set the current tracing level.
**
** Input:
**	level	Tracing level.
**
** Output:
**	None.
**
** Returns:
**	int	Previous tracing level.
**
** History:
**	11-Sep-02 (gordy)
**	    Created.
*/

public synchronized int
set( int level )
{
    int	prior = this.level;
    this.level = level;
    return( prior );
}


} // class TraceLevel

} // class TraceLog

