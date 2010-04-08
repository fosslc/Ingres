/*
** Copyright (c) 1999, 2002 Ingres Corporation All Rights Reserved.
*/

package com.ingres.gcf.util;

/*
** Name: Trace.java
**
** Description:
**	Defines interface for providing tracing output.
**
**  Interfaces
**
**	Trace
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
**	11-Sep-02 (gordy)
**	    Moved to GCF.  Trace log extracted to separate class.
*/


/*
** Name: Trace
**
** Description:
**	Interface which provides level based tracing access to a trace log.
**
**  Public Methods:
**
**	enabled		    Is tracing enabled.
**	getTraceLevel	    Get tracing level.
**	setTraceLevel	    Set tracing level.
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
**	11-Sep-02 (gordy)
**	    Extracted trace log into separate class.  Removed
**	    setTraceLog() and ID versions of getTraceLevel()
**	    and setTraceLevel() (Trace objects initialized with
**	    specific ID, levels for ID's maintained with log).
*/

public interface
Trace
{


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
** Name: setTraceLevel
**
** Description:
**	Sets the current tracing level.
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

