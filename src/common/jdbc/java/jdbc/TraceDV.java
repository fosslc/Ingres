/*
** Copyright (c) 2004 Ingres Corporation
*/

package ca.edbc.jdbc;

/*
** Name: TraceDV.java
**
** Description:
**	Defines class which implements the EdbcTrace interface for
**	internal driver tracing only. 
**
** History:
**	16-Mar-01 (gordy)
**	    Created.
*/

import	java.util.Date;
import	ca.edbc.util.Tracing;


/*
** Name: TraceDV
**
** Description:
**	Implements EdbcTrace interface to support internal driver tracing.
**
**  Interface Methods:
**
**	setTraceLog	Set the tracing output log.
**	enabled		Is tracing enabled?
**	log		Log message to trace log.
**
**  Private Methods:
**
**	init_log	Write initial trace message.
**
** History:
**	16-Mar-01 (gordy)
**	    Created.
*/

public class
TraceDV
    extends Tracing
    implements EdbcTrace, EdbcConst
{


/*
** Name: <class initializer>
**
** Description:
**	Initialize the trace log.
**
** History:
**	27-Mar-01 (gordy)
**	    Created.
*/

static // Class Initializer
{
    init_log();	// Write initial message if log opened by super class.
} // static


/*
** Name: TraceDV
**
** Description:
**	Class constructor.  Set tracing level based on tracing ID.
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
**	16-Mar-01 (gordy)
**	    Created.
*/

public
TraceDV( String id )
{
    super( id );
} // TraceDV


/*
** Name: setTraceLog
**
** Description:
**	Set the trace log.  If the log name is NULL, tracing will 
**	be disabled.
**
**	Writes the initial trace message to the new log.
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
**	27-Mar-01 (gordy)
**	    Created.
*/

public boolean 
setTraceLog( String name )
{
    boolean success = super.setTraceLog( name );
    if ( success )  init_log();
    return( success );
}


/*
** Name: enabled
**
** Description:
**	Returns True if internal driver tracing is enabled (at any level).
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if tracing enabled.
**
** History:
**	16-Mar-01 (gordy)
**	    Created.
*/

public boolean
enabled()
{
    return( enabled( 1 ) );
} // enabled


/*
** Name: log
**
** Description:
**	Writes trace output to trace log (if enabled at any level).
**
** Input:
**	str	Trace message.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	16-Mar-01 (gordy)
**	    Created.
*/

public void
log( String str )
{
    write( 1, str );
    return;
} // log


/*
** Name: init_log
**
** Description:
**	Writes the initial trace message to the output log.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	27-Mar-01 (gordy)
**	    Created.
*/

private static void
init_log()
{
    Date now = new Date();
    println( driverID + ": " + now.toString() );
}


} // class TraceDV
