/*
** Copyright (c) 2002, 2003 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.jdbc;

/*
** Name: IngTrace.java
**
** Description:
**	Defines class which manages the Ingres JDBC drive trace log.
**
**  Classes
**
**	IngTrace
**
** History:
**	31-Oct-02 (gordy)
**	    Created.
**	15-Jul-03 (gordy)
**	    Added runtime configuration.
*/

import	java.util.Date;
import	com.ingres.gcf.util.TraceLog;


/*
** Name: IngTrace
**
** Description:
**	Class which manages the Ingres JDBC driver trace log.
**
**  Public Methods:
**
**	getTraceLog	Returns Ingres JDBC TraceLog.
**	setTraceLog	Set the output trace file.
**
**  Private Data:
**
**	traceLog	Internal driver tracing output.
**
**  Private Methods:
**
**	getDriverTitle	Returns formatted title for driver.
**
** History:
**	31-Oct-02 (gordy)
**	    Created.
**	15-Jul-03 (gordy)
**	    Moved setDriverVers(), majorVers and minorVers to IngConfig.
**	    Added initTraceLog() to centralize trace log creation.
**      14-Feb-08 (rajus01) SIR 119917
**          Added support for JDBC driver patch version for patch builds.
*/

class
IngTrace
    implements IngConst
{

    private static TraceLog	traceLog	= null;	// Ingres trace log


/*
** Name: getTraceLog
**
** Description:
**	Returns the Ingres JDBC trace log.  Initializes trace log
**	on first call.  Driver version should be set with method
**	IngConfig.setDriverVers() prior to calling this method.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	TraceLog	Ingres JDBC trace log.
**
** History:
**	31-Oct-02 (gordy)
**	    Created.
**	15-Jul-03 (gordy)
**	    Moved trace log creation to initTracelog().
*/

public static TraceLog
getTraceLog()
{
    if ( traceLog == null )
    {
	initTraceLog();
	traceLog.write( getDriverTitle() );
    }
    return( traceLog );
} // getTraceLog


/*
** Name: setTraceLog
**
** Description:
**	Set the driver trace file.  Driver version should be set with
**	method IngConfig.setDriverVers() prior to calling this method.
**
** Input:
**	name	Name of log, NULL to disable.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	31-Oct-02 (gordy)
**	    Created.
**	15-Jul-03 (gordy)
**	    Moved trace log creation to initTracelog().
*/

public static void
setTraceLog( String name )
{
    if ( traceLog == null )  initTraceLog();
    traceLog.setTraceLog( name );
    if ( name != null )  traceLog.write( getDriverTitle() );
    return;
}


/*
** Name: initTraceLog
**
** Description:
**	    Initialize the static Ingres driver tracing log.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	15-Jul-03 (gordy)
**	    Created.
*/

private static synchronized void
initTraceLog()
{
    if ( traceLog == null )  
	traceLog = TraceLog.getTraceLog( IngConfig.getConfig() );
    return;
} // initTraceLog


/*
** Name: getDriverTitle
**
** Description:
**	Returns a string identifying the Ingres JDBC driver.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String		Ingres JDBC driver title.
**
** History:
**	31-Oct-02 (gordy)
**	    Created.
**	15-Jul-03 (gordy)
**	    Move driver version information to IngConfig.
*/

private static String
getDriverTitle()
{
    return( ING_DRIVER_NAME + " [" + IngConfig.getMajorVers() + "." + 
	    IngConfig.getMinorVers() + "." + IngConfig.getPatchVers() +
	    "]" + ": " + (new Date()) );
} // getDriverTitle


/*
** Name: IngTrace
**
** Description:
**	Class constructor.  No instances are allowed.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	31-Oct-02 (gordy)
**	    Created.
*/

private IngTrace() {}


} // class IngTrace
