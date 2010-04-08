/*
** Copyright (c) 1999, 2003 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.jdbc;

/*
** Name: IngresDriver.java
**
** Description:
**	Defines class implementing the Ingres JDBC Driver interface.
**
**  Classes
**
**	IngresDriver
**
** History:
**	 5-May-99 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Implemented as wrapper for new generic JDBC driver.
**	15-Jul-03 (gordy)
**	    Added runtime configuration.
**      14-Feb-08 (rajus01) SIR 119917
**          Added support for JDBC driver patch version for patch builds.
*/

import	java.sql.DriverManager;
import	java.sql.Driver;
import	com.ingres.gcf.jdbc.JdbcDrv;
import	com.ingres.gcf.util.Config;
import	com.ingres.gcf.util.TraceLog;


/*
** Name: IngresDriver
**
** Description:
**	The Ingres JDBC Driver class.  Extends and customizes the
**	generic JDBC Driver class JdbcDrv.
**
**	Registers an IngresDriver instance with JDBC DriverManager 
**	when class is loaded.  The driver handles URL's of the form:
**
**	    jdbc:ingres://<host>:<port>[/<database>][;<attr>=<value>]
**
**	Attributes and properties are defined in the EdbcConst
**	interface.  The following attributes have been supported
**	since version 0.1 of the driver.
**
**	    UID=<username>
**	    PWD=<password>
**
**	Attributes may also have the same name as connection
**	properties.  The following properties have been supported
**	since version 0.1 of the driver:
**
**	    database    Ingres target specification: <node>::<db>/<class>
**	    user        Username.
**	    password    Password.
**
**  Overriden Methods:
**
**	loadConfig		Provide config properties to super-class.
**	loadDriverName		Provide Ingres driver name to super-class.
**	loadProtocolID		Provide Ingres protocol ID to super-class.
**	loadTraceName		Provide Ingres tracing name to super-class.
**	loadTraceLog		Provide tracing log to super-class.
**	setTraceLog		Set tracing output.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Implemented as wrapper for new generic JDBC driver.
**	    Extracted static operations from original Driver.
**	15-Jul-03 (gordy)
**	    Replaced setDriverIdentity() with load methods.
**      14-Feb-08 (rajus01) SIR 119917
**          Added driverPatchVersion to setDriverVers(). 
*/

public final class
IngresDriver 
    extends JdbcDrv
    implements Driver, IngConst
{

/*
** Name: <class initializer>
**
** Description:
**	Creates an instance of the class and registers with 
**	the JDBC DriverManager.  Initializes tracing.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
**	28-Mar-01 (gordy)
**	    Create DriverManager tracing.
**	31-Oct-02 (gordy)
**	    Adapted for Ingres JDBC driver.
**	15-Jul-03 (gordy)
**	    Added runtime configuration.
*/

static
{
    String	title = ING_TRACE_NAME + "-Driver";

    /*
    ** Initialize Ingres driver tracing.
    */
    IngConfig.setDriverVers( driverMajorVersion, driverMinorVersion, driverPatchVersion );
    TraceLog traceLog = IngTrace.getTraceLog();

    /*
    ** Create IngresDriver object and register with DriverManager.
    */
    try
    {
	DriverManager.registerDriver( new IngresDriver() );
	DriverManager.println( ING_DRIVER_NAME + ": Version " + 
				driverMajorVersion + "." + driverMinorVersion + 
				"." + driverPatchVersion +
				" (" + driverJdbcVersion + ")" );
	DriverManager.println( driverVendor );
	traceLog.write( title + ": registered" );
    }
    catch( Exception ex )
    {
	DriverManager.println( ING_DRIVER_NAME + ": registration failed!" );
	traceLog.write( title + ": registration failed!" );
    }

} // <class initializer>


/*
** Name: IngresDriver
**
** Description:
**	Class constructor.
**
**	It would be nice to make this constructor private so
**	that only one instance could be created by the static
**	initialization when the class is loaded, but this causes 
**	problems with Microsoft Internet Explorer 4.0 which 
**	doesn't seem to run the static initializer when the 
**	class is simply loaded.  The applet must request a new 
**	instance of the class which requires the constructor to 
**	be public:
**
**	Class.forName("com.ingres.jdbc.IngresDriver").newInstance();
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
**	 5-May-99 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Initialize generic super-class with our run-time info.
**	15-Jul-03 (gordy)
**	    Super-class now loads identity information via load methods.
*/

public
IngresDriver()
{
} // IngresDriver


/*
** The following methods implement the abstract methods of the
** super-class to provide driver identity information.
**
**	loadConfig		Configuration properties.
**	loadDriverName		Full name of driver.
**	loadProtocolID		Protocol accepted by driver.
**	loadTraceName		Name of driver for tracing.
**	loadTraceLog		Tracing log.
**
** History:
**	15-Jul-03 (gordy)
**	    Replaced setDriverIdentity() with load methods.
*/

protected Config
loadConfig()
{ return( IngConfig.getConfig() ); }

protected String
loadDriverName()
{ return( ING_DRIVER_NAME ); }

protected String
loadProtocolID()
{ return( ING_PROTOCOL_ID ); }

protected String
loadTraceName()
{ return( ING_TRACE_NAME ); }

protected TraceLog
loadTraceLog()
{ return( IngTrace.getTraceLog() ); }


/*
** Name: setTraceLog
**
** Description:
**	Set the driver trace log.  Overrides super-class to
**	write initial driver identifier trace to log.
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
**	10-May-00 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Trace log now maintained by Ingres wrapper.
*/

public void
setTraceLog( String name )
{
    IngTrace.setTraceLog( name );
    return;
}

} // class IngresDriver
