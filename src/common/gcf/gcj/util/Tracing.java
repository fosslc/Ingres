/*
** Copyright (c) 1999, 2002 Ingres Corporation All Rights Reserved.
*/

package com.ingres.gcf.util;

/*
** Name: Tracing.java
**
** Description:
**	Defines class which implements the Trace interface to provide
**	access to a trace log with separate trace levels.
**
**  Classes
**
**	Tracing
**
** History:
**	 6-May-99 (gordy)
**	    Created.
**	20-Apr-00 (gordy)
**	    Moved to package util.  Reworked as stand-alone class.
**	16-Mar-01 (gordy)
**	    Separated into Trace interface and Tracing implementation.
**	12-Apr-01 (gordy)
**	    Removed exception tracing (should be self tracing).
**	11-Sep-02 (gordy)
**	    Moved to GCF.  Extracted trace log to TraceLog class.
**	31-Oct-02 (gordy)
**	    Acced method to return TraceLog.
*/


/*
** Name: Tracing
**
** Description:
**	Class which implements Trace interface providing tracing level 
**	access to a trace log.
**
**  Interface Methods:
**
**	enabled		    Is tracing enabled.
**	getTraceLevel	    Get tracing level.
**	setTraceLevel	    Set tracing level.
**	write		    Write to trace log.
**	hexdump		    Write hexdump to trace log.
**
**  Public Methods:
**
**	getTraceLog	    Returns associated TraceLog.
**
**  Private Data:
**
**	trace_log	    Trace log.
**	trace_level	    Tracing level.
**
** History:
**	 6-May-99 (gordy)
**	    Created.
**	20-Apr-00 (gordy)
**	    Reworked as stand alone class rather than base class.
**	    Added per-instance data/members and static registry
**	    to permit multiple tracing levels and ID's.
**	16-Mar-00 (gordy)
**	    Made all methods non-static as required for interface.
**	12-Apr-01 (gordy)
**	    Removed exception tracing (should be self tracing).
**	11-Sep-02 (gordy)
**	    Extracted trace log functionality to separate class.
**	    Tracing levels maintained by trace log.
**	31-Oct-02 (gordy)
**	    Added getTraceLog().
*/

public class
Tracing
    implements Trace
{
    
    private TraceLog		trace_log = null;	// Trace log.
    private TraceLog.TraceLevel	trace_level = null;	// Tracing ID level.


/*
** Name: Tracing
**
** Description:
**	Class constructor.  Instance supports tracing level access
**	for a specific tracing ID.
**
** Input:
**	log	Trace log.
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
**	11-Sep-02 (gordy)
**	    Trace log extracted to separate class.  Tracing levels
**	    maintained with trace log.
*/

public
Tracing( TraceLog log, String id )
{
    this.trace_log = log;
    trace_level = trace_log.getTraceLevel( id );
    return;
} // Tracing


/*
** Name: getTraceLog
**
** Description:
**	Returns the associated TraceLog.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	TraceLog    Associated trace log.
**
** History:
**	31-Oct-02 (gordy)
**	    Created.
*/

public TraceLog
getTraceLog()
{
    return( trace_log );
} // getTraceLog


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
**	11-Sep-02 (gordy)
**	    Trace log extracted to separate class.  Trace level
**	    maintained in trace log class.
*/

public boolean
enabled( int level )
{
    return( trace_log.enabled()  &&  level <= trace_level.get() );
} // enabled


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
**	11-Sep-02 (gordy)
**	    Trace level for ID now maintained in trace log class.
*/

public int
getTraceLevel()
{
    return( trace_level.get() );
} // getTraceLevel


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
**	11-Sep-02 (gordy)
**	    Trace level for ID now maintained in trace log class.
*/

public int
setTraceLevel( int level )
{
    return( trace_level.set( level ) );
} // setTraceLevel


/*
** Name: write
**
** Description:
**	Write a line to the trace log.  A line terminator is appended 
**	to the output.  No action is taken if trace log is not enabled.
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
**	    Trace log extracted to separate class.
*/

public void
write( String str )
{
    trace_log.write( str );
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
**	11-Sep-02 (gordy)
**	    Trace log extracted to separate class.
*/

public void
write( int level, String str )
{
    if ( level <= trace_level.get() )  trace_log.write( str );
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
**	11-Sep-02 (gordy)
**	    Trace log extracted to separate class.
*/

public void 
hexdump( byte buffer[], int offset, int length )
{
    trace_log.hexdump( buffer, offset, length );
    return;
} // hexdump


} // class Tracing

