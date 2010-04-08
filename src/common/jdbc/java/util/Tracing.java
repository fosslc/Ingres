/*
** Copyright (c) 2004 Ingres Corporation
*/

package ca.edbc.util;

/*
** Name: Tracing.java
**
** Description:
**	Class which implements the Trace interface to provide
**	centralized access to a single trace log with separate
**	trace levels.
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
**	16-Mar-01 (gordy)
**	    Separated into Trace interface and Tracing implementation.
**	12-Apr-01 (gordy)
**	    Removed exception tracing (should be self tracing).
**	 5-Feb-02 (gordy)
**	    Added timestamps.
**	20-Mar-02 (gordy)
**	    The System method getProperty() may throw a security exception
**	    (especially in browser applets).  Ignore as if properties not set.
*/

import	java.io.PrintWriter;
import	java.io.FileOutputStream;
import	java.io.FileNotFoundException;
import	java.text.DateFormat;
import	java.text.SimpleDateFormat;
import	java.util.Date;
import	java.util.Hashtable;


/*
** Name: Tracing
**
** Description:
**	Class which implements Trace interface providing access to a 
**	centralized static trace log and registry of tracing levels.  
**	Class instances provide simplified tracing level access to the 
**	trace log.
**
**  Interface Methods:
**
**	setTraceLog	    Set tracing log.
**	setTraceLevel	    Set tracing level.
**	getTraceLevel	    Get tracing level.
**	enabled		    Is tracing enabled.
**	write		    Write to trace log.
**	hexdump		    Write hexdump to trace log.
**
**  Private Methods:
**
**	timestamp	    Returns current timestamp.
**	open_log	    Open named trace log.
**
**  Constants:
**
**	KEY_TRACE	    System properties base key.
**	KEY_TRACE_LOG	    Property key for trace log.
**	KEY_TRACE_TIME	    Property key for tracing timestamps.
**	hex		    Decimal to Hex conversion characters.
**
**  Private Data:
**
**	trace_log	    Global trace log.
**	levels		    Global tracing level registry.
**	trace_time	    Traces include timestamps?
**	df_ts		    Date format for timestamps.
**	hex_buff	    Buffer used by hexdump();
**	trace_level	    Current tracing level.
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
**	16-Mar-00 (gordy)
**	    Made all methods non-static as required for interface.
**	12-Apr-01 (gordy)
**	    Removed exception tracing (should be self tracing).
**	 5-Feb-02 (gordy)
**	    Added timestamp(), KEY_TRACE_TIME, trace_time, df_ts.
*/

public class
Tracing
    implements Trace
{
    

    private static final String	KEY_TRACE = "edbc.trace";
    private static final String	KEY_TRACE_LOG = KEY_TRACE + ".log";
    private static final String	KEY_TRACE_TIME = KEY_TRACE + ".timestamp";

    private static PrintWriter	trace_log = null;
    private static Hashtable	levels = new Hashtable();   // Registry
    private static boolean	trace_time = false;
    private static DateFormat	df_ts = 
				new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS");

    /*
    ** Buffer used by hexdump (requires synchronization)
    */
    private static char		hex_buff[] = new char[ (4 * 16) + 4 ];
    private static final char	hex[] = 
    { '0', '1', '2', '3', '4', '5', '6', '7',
      '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

    private int			trace_level = 0;    // Tracing ID level.


/*
** Name: <class initializer>
**
** Description:
**	Check to see if the trace log has been defined as
**	a system property and initialize appropriately.
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
**	 5-Feb-02 (gordy)
**	    Check to see if timestamps are enabled.
**	20-Mar-02 (gordy)
**	    Handle security exceptions thrown by getProperty().
*/

static // Class Initializer
{
    try
    {
	String value;

	if ( (value = System.getProperty( KEY_TRACE_LOG )) != null )  
	    open_log( value );

	if ( (value = System.getProperty( KEY_TRACE_TIME )) != null )
	    if ( 
		 value.equalsIgnoreCase("true")  ||  
		 value.equalsIgnoreCase("on")  || 
		 value.equalsIgnoreCase("enable")
	       )
	       trace_time = true;
    }
    catch( Exception ignore ) {}

} // static


/*
** Name: Tracing
**
** Description:
**	Class constructor.  Instance supports tracing level access
**	for a specific tracing ID.  The tracing level may be specified
**	as a system property of the form edbc.trace.<ID>=<level>.  If
**	not set at the system level, the trace level registry is used.
**
** Input:
**	id	Tracing ID.
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
*/

public
Tracing( String id )
{
    try
    {
	String  level;

	if ( (level = System.getProperty( KEY_TRACE + "." + id )) != null )
	    trace_level = Integer.parseInt( level );
	else
	    trace_level = getTraceLevel( id );
    }
    catch( Exception ignore ) {}
} // Tracing


/*
** Name: setTraceLog
**
** Description:
**	Set the trace log.  If the log name is NULL, tracing will 
**	be disabled.  A single global trace log is maintained.
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
**	16-Mar-01 (gordy)
**	    Removed static specifier because of Trace interface.
**	    Extracted opening of log to open_log() for static access.
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
	try { open_log( name ); }
	catch( FileNotFoundException ex ) { success = false; }

    return( success );
} // setTraceLog


/*
** Name: setTraceLevel
**
** Description:
**	Set the tracing level for a given trace ID.  Returns
**	the previous tracing level or zero if not previously
**	set.
**
** Input:
**	id	    Trace ID.
**	level	    New tracing level.
**
** Output:
**	None.
**
** Returns:
**	int	    Previous tracing level.
**
** History:
**	 6-May-99 (gordy)
**	    Created.
*/

public int
setTraceLevel( String id, int level )
{
    Integer prev = (Integer)levels.put( id, new Integer(level) );
    return( prev != null ? prev.intValue() : 0 );
} // setTraceLevel


/*
** Name: setTraceLevel
**
** Description:
**	Sets the current tracing level.  Note: does not change
**	the tracing level for a registered ID.
**
** Input:
**	level	    Tracing level.
**
** Output:
**	None.
**
** Returns:
**	int	    Previous tracing level.
**
** History:
**	20-Apr-00 (gordy)
**	    Created.
*/

public int
setTraceLevel( int level )
{
    int old_level = trace_level;
    trace_level = level;
    return( old_level );
} // setTraceLevel


/*
** Name: getTraceLevel
**
** Description:
**	Get the tracing level for a trace ID.  Zero is returned
**	for any ID which has not had its tracing level set.
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
*/

public int
getTraceLevel( String id )
{
    Integer value = (Integer)levels.get( id );
    return( value != null ? value.intValue() : 0 );
} // getTraceLevel


/*
** Name: getTraceLevel
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
**	int	    Current tracing level.
**
** History:
**	20-Apr-00 (gordy)
**	    Created.
*/

public int
getTraceLevel()
{
    return( trace_level );
} // getTraceLevel


/*
** Name: enabled
**
** Description:
**	Returns the current tracing state for a target trace level.
**
** Input:
**	level	    Tracing level.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if tracing enabled,
**
** History:
**	 6-May-99 (gordy)
**	    Created.
*/

public boolean
enabled( int level )
{
    return( trace_log != null  &&  level <= trace_level );
} // enabled


/*
** Name: write
**
** Description:
**	Write a line to the trace log.  A line terminator is appended 
**	to the output.  No action is taken if tracing is not enabled.
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
*/

public void
write( String str )
{
    if ( trace_time )  str = timestamp() + ": " + str;
    println( str );
    return;
} // write


/*
** Name: write
**
** Description:
**	Write a line to the trace log.  A line terminator is appended
**	to the output.  No action is taken if tracing is not enabled.
**
** Input:
**	level	    Tracing level.
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
*/

public void
write( int level, String str )
{
    if ( level <= trace_level )
    {
	if ( trace_time )  str = timestamp() + ": " + str;
	println( str );
    }
    return;
} // write


/*
** Name: hexdump
**
** Description:
**	Format a hex dump of a byte array (subset) and write to
**	the trace file.
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
*/

public void
hexdump( byte buffer[], int offset, int length )
{
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

	    println( new String( hex_buff ) );
	}
    }
    
    return;
} // hexdump


/*
** Name: println
**
** Description:
**	Print an output line to the trace log (if open).
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
*/

protected static void
println( String str )
{
    if ( trace_log != null )  trace_log.println( str );
    return;
}


/*
** Name: timestamp
**
** Description:
**	Returns a tracing timestamp string for the current time.
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
*/

private static String
timestamp()
{
    return( df_ts.format( new Date() ) );
} // timestamp


/*
** Name: open_log
**
** Description:
**	Open a named log file for tracing output.
**
** Input:
**	name	Name of trace log.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	16-Mar-01 (gordy)
**	    Extracted from setTraceLog() for static access.
*/

private static void
open_log( String name )
    throws FileNotFoundException
{
    trace_log = new PrintWriter( new FileOutputStream( name ), true );
    return;
} // open_log


} // class Tracing

