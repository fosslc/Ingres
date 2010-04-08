/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: EdbcInfo.java -- Ingres JDBC Java Driver
**
** Description:
**	Displays the JDBC Driver information.
**
** History:
**	28-Mar-00 (gordy)
**	    Created.
**	31-May-01 (gordy)
**	    Added ability to test low level JDBC Server connections.
*/

import java.sql.*;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;
import ca.edbc.jdbc.EdbcDriver;
import java.applet.Applet;
import java.awt.Graphics;


/*
** Name: EdbcInfo
**
** Description:
**	This class provides the public interface to display
**	JDBC Driver information.  The EDBC Driver is selected
**	by default.  A URL may be used to specify the JDBC
**	Driver and test a DBMS connection.  A low level JDBC
**	Server connection can also be tested.
**
**  Public Methods:
**
**	main	    Application entry point
**
**  Private Data:
**
**	driver	    Class designation for the EDBC Driver.
**	DBC_V0	    DbConn class designation for beta Driver.
**	DBC_V1	    DbConn class designation.
**	default_url URL which matches the EDBC Driver.
**	edbc	    Default driver name.
**	applet_info Info object initialized by init() for paint().
**
**  Private Methods:
**
**	getInfo	    Loads the JDBC Driver information.
**
** History:
**	28-Mar-00 (gordy)
**	    Created.
**	29-Mar-00 (gordy)
**	    Added default driver name.
**	31-May-01 (gordy)
**	    Separated driver info and connection code into driverInfo()
**	    and dbmsConnect() methods.  Added serverConnect() method and
**	    constants DBC_V0, DBC_V1.
*/

public class 
EdbcInfo
    extends Applet
{

    private static final String	    driver = "ca.edbc.jdbc.EdbcDriver";
    private static final String	    DBC_V0 = "ca.edbc.jdbc.DbConn";
    private static final String	    DBC_V1 = "ca.edbc.io.DbConn";
    private static final String	    default_url = "jdbc:edbc://host:port/db";
    private static final String	    edbc = "CA-EDBC JDBC Driver";
    private DrvInfo		    applet_info = null;


/*
** Name: main
**
** Description:
**	Application entry point for display driver info on the
**	system console.  A URL may be provided as a single 
**	command argument.  Alternatively, the JDBC server host
**	and port may be provided as two command line arguments.
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
*/

public static void 
main( String args[] ) 
{
    DrvInfo info;
    String  error, msg;

    if ( args.length == 0 )
    {
	msg = "Error obtaining driver info";
	info = driverInfo();
    }
    else  if ( args.length == 1 )
    {
	msg = "Error connecting to URL";
	info = dbmsConnect( args[ 0 ] );
    }
    else  if ( args.length == 2 )
    {
	String target = args[ 0 ] + ":" + args[ 1 ];
	msg = "Error connecting to JDBC Server";

	if ( (info = serverConnect( DBC_V1, target )) == null  &&
	     (info = serverConnect( DBC_V0, target )) == null )
	{
	    System.out.println( "" );
	    return;
	}
    }
    else
    {
	System.out.println( "Invalid arguments!" );
	return;
    }

    if ( (error = info.error()) != null )
	System.out.println( msg + ": " + error );
    else
	System.out.println( "Driver version: " + info.version() );
}


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
**	28-Mar-00 (gordy)
**	    Created.
**	31-May-01 (gordy)
**	    Separated getInfo() into driverInfo() and dbmsConnect().
*/

public void
init()
{
    String url = getParameter( "URL" );
    applet_info = (url == null) ? driverInfo() : dbmsConnect( url );
    return;
}

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
*/

public void 
paint( Graphics g )
{
    String error;

    if ( applet_info != null )
	if ( (error = applet_info.error()) != null )
	    g.drawString( "Error obtaining driver info: " + error, 5, 20 );
	else
	    g.drawString( "Driver version: " + applet_info.version(), 5, 20 );
    else
	g.drawString( "Driver info not loaded?", 5, 20 );

    return;
}

/*
** Name: driverInfo
**
** Description:
**	Returns the JDBC Driver info for the EDBC Driver.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	DrvInfo	The driver information.
**
** History:
**	28-Mar-00 (gordy)
**	    Created.
**	29-Mar-00 (gordy)
**	    Set driver name in the driver info.
**	31-May-01 (gordy)
**	    Extracted connection code to dbmsConnect().
*/

private static DrvInfo
driverInfo()
{
    DrvInfo info;
    Driver  drv;

    try
    {
	Class.forName( driver ).newInstance();
    }
    catch( Exception ex1 )
    {
	return( new DrvInfo( "Class not found: " + driver ) );
    }

    try
    {
	drv = DriverManager.getDriver( default_url );
    }
    catch( Exception ex2 )
    {
	return( new DrvInfo( "No driver for URL: " + default_url ) );
    }

    info = new DrvInfo( drv.getMajorVersion(), drv.getMinorVersion() );
    info.setName( edbc );
    return( info );
}

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
**	DrvInfo	The driver information.
**
** History:
**	31-May-01 (gordy)
**	    Extracted from driverInfo().
*/

private static DrvInfo
dbmsConnect( String url )
{
    DrvInfo info;

    try
    {
	Class.forName( driver ).newInstance();
    }
    catch( Exception ex1 )
    {
	return( new DrvInfo( "Class not found: " + driver ) );
    }

    try
    {
	Connection conn = DriverManager.getConnection( url );

	try
	{
	    DatabaseMetaData meta = conn.getMetaData();
	    info = new DrvInfo( meta.getDriverMajorVersion(), 
				meta.getDriverMinorVersion() );
	    info.setName( meta.getDriverName() );
	}
	catch( Exception ex ) 
	{
	    info = new DrvInfo( ex.getMessage() ); // Shouldn't happen!
	}
	finally
	{
	    conn.close();
	}
    }
    catch( Exception conn_ex )
    {
	info = new DrvInfo( conn_ex.getMessage() );
    }

    return( info );
}

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
**	dbc_name    DbConn class name.
**	target	    Server host and port (host:port).
**
** Output:
**	None.
**
** Returns:
**	DrvInfo	The driver information or NULL.
**
** History:
**	31-May-01 (gordy)
**	    Created.
*/

private static DrvInfo
serverConnect( String dbc_name, String target )
{
    Class   parm_type[] = new Class[ 3 ];
    Object  parm_val[] = new Object[ 3 ];
    Class   no_parm_type[] = new Class[ 0 ];
    Object  no_parm_val[] = new Object[ 0 ];
    byte    buff[] = new byte[ 3 ];

    buff[0] = buff[1] = buff[2] = 1;	// MSG protocol level 1.

    parm_type[ 0 ] = target.getClass();
    parm_val[ 0 ] = target;
    parm_type[ 1 ] = buff.getClass();
    parm_val[ 1 ] = buff;
    parm_type[ 2 ] = Short.TYPE;
    parm_val[ 2 ] = new Short( (short)3 );

    try
    {
	Class dbc_class = Class.forName( dbc_name );
	Constructor dbc_const = dbc_class.getConstructor( parm_type );
	Method dbc_close = dbc_class.getMethod( "close", no_parm_type );

	try
	{
	    Object dbc = dbc_const.newInstance( parm_val );
	    dbc_close.invoke( dbc, no_parm_val );
	}
	catch( InvocationTargetException ite )
	{
	    Throwable ex = ite.getTargetException();
	    return( new DrvInfo( ex.getMessage() ) );
	}
	catch( Exception ex4 )
	{
	    return( new DrvInfo( "Driver not accessible: " + driver ) );
	}
    }
    catch( Exception ex3 )
    {
	return( null );
    }

    return( driverInfo() );
}

} // class EdbcInfo


/*
** Name: DrvInfo
**
** Description:
**	Class which holds the JDBC Driver information.
**
**  Public Methods:
**
**	setName	    Set the name of the driver.
**	error	    Returns error message.
**	version	    Returns driver version.
**
**  Private Data:
**
**	errmsg	    Error message.
**	major	    Driver major version.
**	minor	    Driver minor version.
**	name	    Driver name.
**
** History:
**	28-Mar-00 (gordy)
**	    Created.
**	29-Mar-00 (gordy)
**	    Added driver name to version.
*/

class DrvInfo
{

    private String  errmsg = null;
    private int	    major = 0;
    private int	    minor = 0;
    private String  name = null;

/*
** Name: DrvInfo
**
** Description:
**	Class constructor which saves any error message
**	produced while loading driver information.
**
** Input:
**	errmsg	    Error message.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	28-Mar-00 (gordy)
**	    Created.
*/

DrvInfo( String errmsg )
{
    this.errmsg = errmsg;
}

/*
** Name: DrvInfo
**
** Description:
**	Class constructor which saves the driver major and
**	minor version information.
**
** Input:
**	major	    Driver major version.
**	minor	    Driver minor version.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	28-Mar-00 (gordy)
**	    Created.
*/

DrvInfo( int major, int minor )
{
    this.major = major;
    this.minor = minor;
}

/*
** Name: setName
**
** Description:
**	Saves the name of the driver.
**
** Input:
**	name	Driver name.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	29-Mar-00 (gordy)
**	    Created.
*/

public void
setName( String name )
{
    this.name = name;
    return;
}

/*
** Name: error
**
** Description:
**	Returns any error message produced while obtaining
**	the driver information.  Returns NULL if no error.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	    Error message or NULL.
**
** History:
**	28-Mar-00 (gordy)
**	    Created.
*/

public String
error()
{
    return( errmsg );
}

/*
** Name: version
**
** Description:
**	Returns the driver version as a string in the
**	form 'major.minor'.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	    Driver version.
**
** History:
**	28-Mar-00 (gordy)
**	    Created.
**	29-Mar-00 (gordy)
**	    Combine driver name into version if available.
*/

public String
version()
{
    String vers;

    if ( name != null )
	vers = name + " " + major + "." + minor;
    else
	vers = "" + major + "." + minor;

    return( vers );
}

} // class DrvInfo
