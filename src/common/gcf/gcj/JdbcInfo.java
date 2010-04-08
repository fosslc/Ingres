/*
** Copyright (c) 2003, 2004 Ingres Corporation All Rights Reserved.
*/

/*
** Name: JdbcInfo.java -- Ingres JDBC Java Driver
**
** Description:
**	Displays the JDBC Driver information.
**
**	Usage: java JdbcInfo [url] | [host port] | [-class [class]]
**
** History:
**	28-Mar-03 (gordy)
**	    Created.
**	26-Jan-04 (gordy)
**	    Separated application and applet functionality.  Use
**	    reflection to access messaging interface.
**	24-Jan-06 (gordy)
**	    Update package references for re-branding to Ingres Corporation.
**	 6-Oct-06 (gordy)
**	    Add options to display/provide driver class.
**	14-Feb-08 (rajus01) SIR 119917
**	    Added support for JDBC driver patch version for patch builds.
*/

import	java.sql.*;
import	java.applet.Applet;
import	java.awt.Graphics;
import	java.lang.reflect.Constructor;
import	java.lang.reflect.Method;
import	java.lang.reflect.InvocationTargetException;


/*
** Name: JdbcInfo
**
** Description:
**	This class provides the public interface to display JDBC 
**	Driver information.  The Ingres Driver is selected by default.  
**	A URL may be used to specify the JDBC Driver and test a DBMS 
**	connection.  A low level JDBC Server connection can also be 
**	tested.
**
**  Public Methods:
**
**	main			Application entry point
**	init			Initialize applet interface.
**	paint			Display information for applet.
**
**  Private Data:
**
**	driver_class		Class designation for the Ingres Driver.
**	default_url		URL which matches the Ingres Driver.
**	MSG_CLASS_NAME		Name of messaging class.
**	LOG_CLASS_NAME		Name of logging class.
**	LOG_GET_METHOD		Name of logging factory method.
**	MSG_CLOSE_METHOD	Name of messaging close method.
**	message			Applet result/status message
**	ex			Applet runtime exception
**
**  Private Methods:
**
**	driverInfo		Display driver info.
**	driverClass		Display driver class.
**	dbmsConnect		Test DBMS connection, display meta-data info.
**	serverConnect		Test DAS connection.
**	loadDriver		Load driver (applet).
**	testConnect		Test DBMS connection (applet).
**
** History:
**	28-Mar-03 (gordy)
**	    Created.
**	26-Jan-04 (gordy)
**	    Replace text/display with separate methods and fields for applets.
**	24-Jan-06 (gordy)
**	    Update package references for re-branding to Ingres Corporation.
**	15-Sep-06 (gordy)
**	    CharSet class name no longer needed.
**	 6-Oct-06 (gordy)
**	    Added driverInfo( class ) driverClass() methods.
*/

public class 
JdbcInfo
    extends Applet
{

    private static final String	driver_class = 
					"com.ingres.jdbc.IngresDriver";
    private static final String	default_url = 
					"jdbc:ingres://host:port/db";
    
    private static final String MSG_CLASS_NAME = 
					"com.ingres.gcf.dam.MsgConn";
    private static final String LOG_CLASS_NAME = 
					"com.ingres.gcf.util.TraceLog";
    private static final String LOG_GET_METHOD		= "getTraceLog";
    private static final String MSG_CLOSE_METHOD	= "close";
    private static final String PATCH_VERS_METHOD	= "getPatchVersion";

    private String	message = "";	// Applet result/status message
    private Exception	ex = null;	// Applet runtime exception
    

  
/*
** Name: main
**
** Description:
**	Application entry point for display driver info on the
**	system console.  
**
**		java JdbcInfo
**
**	A URL may be provided as a single command argument.  
**
**		java JdbcInfo <URL>
**
**	Alternatively, the JDBC server host and port may be provided 
**	as two command line arguments.
**
**		java JdbcInfo <host> <port>
**
** Input:
**	args	    Command line arguments.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	28-Mar-00 (gordy)
**	    Created.
**	31-May-01 (gordy)
**	    Added low level server connect with two command line
**	    arguments: target host and port.
**	26-Jan-04 (gordy)
**	    Separated applet processing.  No longer need instance
**	    or call to display().
**	 6-Oct-06 (gordy)
**	    Added support for -class argument.  Extracted help/usage.
*/

public static void 
main( String args[] )
{
    switch( args.length )
    {
    case 0 :
	driverInfo();
	break;

    case 1 :
	if ( args[0].equals( "-class" ) )
	    driverClass();
	else
	    dbmsConnect( args[0] );
	break;

    case 2 :
	if ( args[0].equals( "-class" ) )
	    driverInfo( args[1] );
	else
	    serverConnect( args[0], args[1] );
	break;

    default :
	System.out.println( 
		"Invalid arguments!" );
	System.out.println( 
		"  java JdbcInfo               : display driver information" );
	System.out.println( 
		"  java JdbcInfo <URL>         : test DBMS connection" );
	System.out.println( 
		"  java JdbcInfo <host> <port> : test server connection" );
	break;
    }

    return;
} // main


/*
** Name: driverInfo
**
** Description:
**	Returns the JDBC Driver info for the Ingres Driver.
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
**	 6-Oct-06 (gordy)
**	    Created.
*/

private static void
driverInfo()
{ driverInfo( driver_class ); }


/*
** Name: driverInfo
**
** Description:
**	Returns the JDBC Driver info for the specific driver.
**
** Input:
**	class_name	Name of driver class.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	28-Mar-03 (gordy)
**	    Created.
**	26-Jan-04 (gordy)
**	    No longer supports applets.  Made static.  Does direct output.
**	 6-Oct-06 (gordy)
**	    Added ability to specify the driver class.
**	14-Feb-08 (rajus01) SIR 119917
**	    Added support for JDBC driver patch version for patch builds.
*/

private static void
driverInfo( String class_name )
{
    Driver  drv;
    boolean patch_method_exists = false;
    
    try { drv = (Driver)Class.forName( class_name ).newInstance(); }
    catch( Exception ex )
    {
	System.out.println("Error loading driver class '" + class_name + "':");
	System.out.println( ex.getMessage() );
	return;
    }
    
    Method[] methods = drv.getClass().getMethods();
    for( int i= 0; i < methods.length; i++ )
        if( methods[i].getName().equals(PATCH_VERS_METHOD) )
	    patch_method_exists = true;

    System.out.println( drv.toString() + " [" + 
			drv.getMajorVersion() + "." + 
			drv.getMinorVersion() + 
			((( drv instanceof com.ingres.jdbc.IngresDriver ) &&
			 patch_method_exists) ?
			"." + 
			((com.ingres.jdbc.IngresDriver)drv).getPatchVersion()
			+ "]" : "]" ));
    
    /*
    ** If this is the default driver class, make sure
    ** it successfully responds to the default URL.
    */
    if ( class_name.equals( driver_class ) )
	try
	{
	    Driver url_drv = DriverManager.getDriver( default_url );

	    /*
	    ** There is a driver for the default URL,
	    ** but is it the driver we are testing?
	    */
	    if ( url_drv.getClass() != drv.getClass() )
	    {
		System.out.println( "Class: " + drv.getClass().getName() );
		System.out.println();
		System.out.println( "Driver servicing default URL '" + 
				    default_url + "':" );
		System.out.println( drv.toString() + " [" + 
			drv.getMajorVersion() + "." + 
			drv.getMinorVersion() + 
			((( drv instanceof com.ingres.jdbc.IngresDriver ) &&
			 patch_method_exists) ?
			"." + 
			((com.ingres.jdbc.IngresDriver)drv).getPatchVersion()
			+ "]" : "]" ));

		System.out.println( "    Class: " + 
				    url_drv.getClass().getName() );
	    }
	}
	catch( Exception ex )
	{
	    System.out.println( "Error accessing driver for URL '" + 
				default_url + "':" );
	    System.out.println( ex.getMessage() );
	}

    return;
} // driverInfo


/*
** Name: driverClass
**
** Description:
**	Display class of default driver.
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
**	 6-Oct-06 (gordy)
**	    Created.
*/

private static void
driverClass()
{
    Driver  drv;
    
    try { drv = (Driver)Class.forName( driver_class ).newInstance(); }
    catch( Exception ex )
    {
	System.out.println( "Error loading driver class '" + 
			    driver_class + "':" );
	System.out.println( ex.getMessage() );
	return;
    }

    System.out.println( drv.getClass().getName() );
    return;
} // driverClass


/*
** Name: dbmsConnect
**
** Description:
**	Returns the JDBC Driver info for a given URL.
**
** Input:
**	url	A URL for the desired driver.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	28-Mar-03 (gordy)
**	    Created.
**	26-Jan-04 (gordy)
**	    No longer supports applets.  Made static.  Does direct output.
**	14-Feb-08 (rajus01) SIR 119917
**	    Added support for JDBC driver patch version for patch builds.
**	    Use DatabaseMetaData.getDriverVersion() to report the driver
**	    version.
*/

private static void
dbmsConnect( String url )
{
    Connection	conn;
    Driver	drv;
    
    try { Class.forName( driver_class ).newInstance(); }
    catch( Exception ex )
    {
	System.out.println( "Error loading class '" + driver_class + "':" );
	System.out.println( ex.getMessage() );
	return;
    }

    try { conn = DriverManager.getConnection( url ); }
    catch( Exception ex )
    {
	System.out.println( "Error establishing connection:" );
	System.out.println( ex.getMessage() );
	return;
    }

    try { drv = DriverManager.getDriver( url ); }
    catch( Exception ex ) { drv = null; }
    
    try
    {
        DatabaseMetaData meta = conn.getMetaData();
	System.out.println( meta.getDriverName() + " [" + 
			    meta.getDriverVersion() + "]: (JDBC " +
			    meta.getJDBCMajorVersion() + "." +
			    meta.getJDBCMinorVersion() + ")" );

	System.out.println( meta.getDatabaseProductName() + " [" +
			    meta.getDatabaseMajorVersion() + "." +
			    meta.getDatabaseMinorVersion() + "]: " +
			    meta.getDatabaseProductVersion() );
    }
    catch( Exception ex )
    {
	System.out.println( "Error accessing DBMS Meta-Data:" );
        System.out.println( ex.getMessage() );
    }
    finally
    {
	try { conn.close(); } catch( Exception ignore ) {}
    }
    
    return;
} // dbmsConnect


/*
** Name: serverConnect
**
** Description:
**	Attempts a low level connect to a target JDBC Server.
**	Caller must provide DbConn class name.  Returns NULL
**	if DbConn class does not exist (allows retry with
**	different class).  If connection fails, returns info
**	with error message.  If connection succeeds, returns
**	output of driverInfo().
**
** Input:
**	host	host.
**	port	Port.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	28-Mar-03 (gordy)
**	    Created.
**	26-Jan-04 (gordy)
**	    No longer supports applets.  Made static.  Does direct output.
**	    Use reflection to remove direct dependency on JDBC classes.
**	15-Sep-06 (gordy)
**	    MSG class no longer takes CHARSET parameter.
*/

private static void
serverConnect( String host, String port )
{
    String	target = host + ":" + port;
    byte	parms[] = { 1, 1, 1 };
    byte	buff[][] = { parms };
    Class	msg_parm_typ[] = new Class[ 3 ];
    Object	msg_parm_val[] = new Object[ 3 ];
    Class	no_parm_typ[] = new Class[ 0 ];
    Object	no_parm_val[] = new Object[ 0 ];
    
    try
    {
	Class	msg_class = Class.forName( MSG_CLASS_NAME );
	Class	log_class = Class.forName( LOG_CLASS_NAME );
	Method	log_get = log_class.getMethod( LOG_GET_METHOD, no_parm_typ );
	
	msg_parm_typ[ 0 ] = target.getClass();
	msg_parm_val[ 0 ] = target;
	msg_parm_typ[ 1 ] = buff.getClass();
	msg_parm_val[ 1 ] = buff;
	msg_parm_typ[ 2 ] = log_class;
	msg_parm_val[ 2 ] = log_get.invoke( null, no_parm_val );
	
	Constructor	msg_constructor = 
				msg_class.getConstructor( msg_parm_typ );
	Method		msg_close = 
				msg_class.getMethod( MSG_CLOSE_METHOD, 
						     no_parm_typ );
	
	Object msg = msg_constructor.newInstance( msg_parm_val );
	msg_close.invoke( msg, no_parm_val );
	System.out.println( "Connected successfully to '" + target + "'" );
    }
    catch( InvocationTargetException ite )
    {
	System.out.println( "Error connecting to '" + target + "':" );
	System.out.println( ite.getTargetException().getMessage() );
    }
    catch( Exception ex )
    {
	System.out.println( "Error accessing driver components:" );
	System.out.println( ex.getMessage() );
    }

    return;
} // serverConnect


/*
** Name: init
**
** Description:
**	Applet entry point for loading driver information.
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
**	28-Mar-03 (gordy)
**	    Created.
**	26-Jan-04 (gordy)
**	    Functionality separated from application.
*/

public void
init()
{
    String url = getParameter( "URL" );
    
    message = "Executing . . .";
    ex = null;
    
    if ( url == null ) 
	loadDriver();
    else
	testConnect( url );
    
    return;
} // init


/*
** Name: paint
**
** Description:
**	Overrides the applets paint method to display the
**	JDBC Driver information.
**
** Input:
**	g	Graphics object for display.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	28-Mar-00 (gordy)
**	    Created.
**	26-Jan-04 (gordy)
**	    Functionality separated from application.
*/

public void 
paint( Graphics g )
{
    int height = g.getFontMetrics().getHeight();
    int offset = g.getFontMetrics().getLeading() * 2;
    
    g.drawString( message, offset, offset + height );
    if ( ex != null )  
	g.drawString( ex.getMessage(), offset, offset + (height * 2) );
    return;
} // paint


/*
** Name: loadDriver
**
** Description:
**	Returns the JDBC Driver info for the Ingres Driver.
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
**	26-Jan-04 (gordy)
**	    Created.
*/

private void
loadDriver()
{
    Driver  drv;
    
    try { Class.forName( driver_class ).newInstance(); }
    catch( Exception ex )
    {
	message = "Error loading class '" + driver_class + "'";
	this.ex = ex;
	return;
    }

    try
    {
	drv = DriverManager.getDriver( default_url );
	message = drv.toString() + " [" + 
		  drv.getMajorVersion() + "." +	drv.getMinorVersion() + "]";
    }
    catch( Exception ex )
    {
	message = "Error accessing driver for URL '" + default_url + "'";
	this.ex = ex;
    }

    return;
} // loadDriver


/*
** Name: testConnect
**
** Description:
**	Returns the JDBC Driver info for a given URL.
**
** Input:
**	url	A URL for the desired driver.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	26-Jan-04 (gordy)
**	    Created.
*/

private void
testConnect( String url )
{
    Connection conn;
    
    try { Class.forName( driver_class ).newInstance(); }
    catch( Exception ex )
    {
	message = "Error loading class '" + driver_class + "'";
	this.ex = ex;
	return;
    }

    try { conn = DriverManager.getConnection( url ); }
    catch( Exception ex )
    {
	message = "Error establishing connection";
	this.ex = ex;
	return;
    }

    try
    {
        DatabaseMetaData meta = conn.getMetaData();
	message = meta.getDatabaseProductName() + " [" +
		  meta.getDatabaseMajorVersion() + "." +
		  meta.getDatabaseMinorVersion() + "]: " +
		  meta.getDatabaseProductVersion();
    }
    catch( Exception ex )
    {
	message = "Error accessing DBMS Meta-Data";
        this.ex = ex;
    }
    finally
    {
	try { conn.close(); } catch( Exception ignore ) {}
    }
    
    return;
} // testConnect

} // class JdbcInfo

