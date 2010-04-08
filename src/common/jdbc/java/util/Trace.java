/*
** Copyright (c) 2004 Ingres Corporation
*/

package ca.edbc.util;

/*
** Name: Trace.java
**
** Description:
**	Defines the Trace interface for providing tracing 
**	capabilities in the EDBC packages.
**
** History:
**	 6-May-99 (gordy)
**	    Created.
**	20-Apr-00 (gordy)
**	    Moved to package util.  Reworked as stand-alone class.
**	16-Mar-01 (gordy)
**	    Separated into interface and implementation class.
**	12-Apr-01 (gordy)
**	    Removed exception tracing (should be self tracing).
*/


/*
** Name: Trace
**
** Description:
**	Interface which provides access to a trace log, a registry 
**	of tracing levels, and level based tracing.
**
**  Public Methods:
**
**	setTraceLog	    Set tracing log.
**	setTraceLevel	    Set tracing level.
**	getTraceLevel	    Get tracing level.
**	enabled		    Is tracing enabled.
**	write		    Write to trace log.
**	hexdump		    Write hexdump to trace log.
**
** History:
**	 6-May-99 (gordy)
**	    Created.
**	16-Mar-01 (gordy)
**	    Separated into interface and implementation class.
**	12-Apr-01 (gordy)
**	    Removed exception tracing (should be self tracing).
*/

public interface
Trace
{


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

boolean setTraceLog( String name );


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

int setTraceLevel( String id, int level );


/*
** Name: setTraceLevel
**
** Description:
**	Sets the current tracing level.  Note: does not change
**	the tracing level for a registered trace ID.
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

int setTraceLevel( int level );


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

int getTraceLevel( String id );


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

int getTraceLevel();


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

boolean enabled( int level );


/*
** Name: write
**
** Description:
**	Write a line to the trace log.  A line terminator is appended 
**	to the output.  No action taken if trace log is not open.
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

void write( String str );


/*
** Name: write
**
** Description:
**	Write a line to the trace log if tracing enabled at the
**	requested level.  A line terminator is appended to the
**	output.  No action taken if trace log is not open.
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
*/

void write( int level, String str );


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

void hexdump( byte buffer[], int offset, int length );


} // interface Trace

