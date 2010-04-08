/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbc;

/*
** Name: EdbcDriver.java
**
** Description:
**	Implements the EDBC JDBC Java driver class: EdbcDriver.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
**	13-Sep-99 (gordy)
**	    Implemented error code support.
**	 2-May-00 (gordy)
**	    Re-organized packaging.
**	19-May-00 (gordy)
**	    Added select_loops status field and associated method.
**	    Initialize connection select loop support to driver setting.
**	 4-Oct-00 (gordy)
**	    Standardized internal tracing.
**	28-Mar-01 (gordy)
**	    Connection establishement code extracted to EdbcConnect class.
**	    DriverManager tracing now extension to internal tracing.
**	11-Apr-01 (gordy)
**	    Pass DriverManager connection timeout to EdbcConnect().
**	20-Aug-01 (gordy)
**	    Removed method to set driver select loops (use connection
**	    property instead).
**	 5-Feb-02 (gordy)
**	    Don't trace method parameters which may contain passwords.
*/

import	java.util.Date;
import	java.util.Properties;
import	java.sql.DriverManager;
import	java.sql.Driver;
import	java.sql.DriverPropertyInfo;
import	java.sql.Connection;
import	java.sql.SQLException;
import	ca.edbc.util.EdbcEx;
import	ca.edbc.util.EdbcRsrc;


/*
** Name: EdbcDriver
**
** Description:
**	EDBC JDBC Java driver class which implements the
**	JDBC Driver interface.  The static class initializer
**	creates an instance of the driver and registers it
**	with the JDBC DriverManager.  This driver handles 
**	URL's of the form:
**
**	    jdbc:edbc://<host>:<port>[/<database>][;<attr>=<value>]
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
**	    database	Ingres target specification <vnode>::<db>/<class>
**	    user	Username.
**	    password	Password.
**
**  Interface Methods:
**
**	jdbcCompliant	    Is driver compliant with the JDBC API.
**	getMajorVersion	    Primary driver version.
**	getMinorVersion	    Secondar driver version.
**	acceptsURL	    Check if URL is handled by this driver.
**	getPropertyInfo	    Determine properties required by driver.
**	connect		    Open a new connection.
**
**  Public Methods:
**
**	setTraceLog	    Set the trace output log.
**	setTraceLevel	    Set a tracing level.
**
**  Constants:
**
**	protocol	    Driver's protocol ID.
**
**  Private Data:
**
**	title		    Class title for tracing.
**	tr_id		    Class id for tracing.
**	trace		    DriverManager tracing.
**
**  Private Methods:
**
**	accept		    Check if URL is handled by this driver.
**	getProperties	    Combine URL attributes and a property set.
**	getHost		    Parse URL and return host.
**	getDatabase	    Parse URL and return database.
**	getAttributes	    Parse URL and return attributes.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
**	19-May-00 (gordy)
**	    Added select_loops status field and associated method.
**	 4-Oct-00 (gordy)
**	    Added trace id, tr_id, to standardize internal tracing.
**	28-Mar-01 (gordy)
**	    Driver ID and protocol moved to EdbcConst.  Driver trace id  
**	    now defined by tracing interface.
**	20-Aug-01 (gordy)
**	    Removed method to set driver select loops (use connection
**	    property instead).	    
*/

public class
EdbcDriver 
    implements	java.sql.Driver, EdbcErr, EdbcConst
{

    private static final String	title = shortTitle + "-Driver";
    private static final String	tr_id = "Drv";
    private static EdbcTrace	trace = new TraceDM( EdbcTrace.edbcTraceID );


/*
** Name: <class initializer>
**
** Description:
**	Creates an instance of the class and registers with 
**	the JDBC DriverManager.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
**	28-Mar-01 (gordy)
**	    Create DriverManager tracing.
*/

static // Class Initializer
{
    /*
    ** Write initial trace to DriverManager log.  The internal
    ** trace log will write its own initial trace, so write
    ** directly to the Drivermanager log.
    */
    Date now = new Date();
    DriverManager.println( driverID + ": loaded " + now.toString() );

    /*
    ** Load the Locale specific descriptions for connection properties.
    */
    for( int prop = 0; prop < propInfo.length; prop++ )
	try 
	{ 
	    propInfo[prop].desc = EdbcRsrc.getResource().getString(
							propInfo[prop].rsrcID ); 
	}
	catch( Exception ignore ){}

    /*
    ** Create an IngresDriver object and register
    ** it with the DriverManager.
    */
    try
    {
	DriverManager.registerDriver( new EdbcDriver() );
	if ( trace.enabled( 1 ) )  trace.write( title + ": registered" );
    }
    catch( Exception e )
    {
	if ( trace.enabled() )  trace.log( title + ": registration failed!" );
    }
} // static

/*
** Name: EdbcDriver
**
** Description:
**	Class constructor.  
**
**	It would be nice to make this constructor protected so
**	that only one instance could be created by the static
**	initialization when the class is loaded, but this causes 
**	problems with Microsoft Internet Explorer 4.0 which 
**	doesn't seem to run the static initializer when the 
**	class is simply loaded.  The applet must request a new 
**	instance of the class which requires the constructor to 
**	be public:
**
**	Class.forName("ca.edbc.jdbc.EdbcDriver").newInstance();
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
*/

public
EdbcDriver()
{
} // EdbcDriver

/*
** Name: jdbcCompliant
**
** Description:
**	Returns driver compliance with JDBC API.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if JDBC API compliant, false otherwise.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
*/

public boolean
jdbcCompliant()
{
    if ( trace.enabled() )  trace.log( title + ".jdbcCompliant(): " + true );
    return( true );
} // jdbcCompliant

/*
** Name: getMajorVersion
**
** Description:
**	Returns driver primary version.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Major version.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
*/

public int
getMajorVersion()
{
    if ( trace.enabled() )  
	trace.log( title + ".getMajorVersion(): " + majorVersion );
    return( majorVersion );
} // getMajorVersion

/*
** Name: getMinorVersion
**
** Description:
**	Returns driver secondary version.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Minor version.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
*/

public int
getMinorVersion()
{
    if ( trace.enabled() )  
	trace.log( title + ".getMinorVersion(): " + minorVersion );
    return( minorVersion );
} // getMinorVersion

/*
** Name: acceptsURL
**
** Description:
**	Returns an indication that the URL provided is handled by
**	this driver.  This driver handles URL's of the form:
**
**	    jdbc:edbc:<sub-name>
**
** Input:
**	url	    Connection target URL.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if URL accepted, false otherwise.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
**	 5-Feb-02 (gordy)
**	    Removed parameters from trace to suppress display of password.
*/

public boolean
acceptsURL( String url )
    throws SQLException
{
    boolean ok = accept( url );
    if ( trace.enabled() )  trace.log( title + ".acceptsURL(): " + ok );
    return( ok );
} // acceptsURL

/*
** Name: getPropertyInfo
**
** Description:
**	Returns the properties required by the driver which are not
**	already present in the URL string or current property set.
**
** Input:
**	url	    Connection target URL
**	info	    Current property set.
**
** Output:
**	None.
**
** Returns:
**	DriverPropertyInfo[]	Array of driver properties.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
**	 5-Feb-02 (gordy)
**	    Removed parameters from trace to suppress display of password.
*/

public DriverPropertyInfo[]	
getPropertyInfo( String url, Properties info )
    throws SQLException
{
    DriverPropertyInfo	dp[];

    if ( trace.enabled() )  trace.log( title + ".getPropertyInfo()" );

    if ( ! accept( url ) )  
    {
	if ( trace.enabled() )  
	    trace.log( title + ".getPropertyInfo(): URL not accepted!" );
	throw EdbcEx.get( E_JD0003_BAD_URL );
    }

    info = getProperties( url, info );
    dp = new DriverPropertyInfo[ propInfo.length ];

    for( int i = 0; i < propInfo.length; i++ )
    {
	String value = info.getProperty( propInfo[ i ].name );
	dp[ i ] = new DriverPropertyInfo( propInfo[ i ].name, value );
	dp[ i ].description = propInfo[ i ].desc;
	dp[ i ].required = propInfo[ i ].req;
    }

    return( dp );
} // getPropertyInfo

/*
** Name: connect
**
** Description:
**	Open a new connection, allocate and return a JDBC Connection
**	object.  
**
** Input:
**	url	Connection target URL.
**	info	Driver property value pairs.
**
** Output:
**	None.
**
** Returns:
**	Connection  A new connection object, null if URL not supported.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
**	19-May-00 (gordy)
**	    Initialize connection select loop status with driver setting.
**	28-Mar-01 (gordy)
**	    Connection establishement code extracted to EdbcConnect class.
**	11-Apr-01 (gordy)
**	    Added timeout parameter to EdbcConnect() so that DriverManager
**	    reference could be localized to this class.
**	 5-Feb-02 (gordy)
**	    Removed parameters from trace to suppress display of password.
*/

public Connection
connect( String url, Properties info )
    throws SQLException
{
    EdbcConnect	conn;

    if ( trace.enabled() )  trace.log( title + ".connect()" );

    if ( ! accept( url ) )  
    {
	if ( trace.enabled() )  
	    trace.log( title + ".connect(): URL not accepted!" );
	return( null );
    }

    try
    {
	/*
	** Convert URL and combine with properties.
	*/
	info = getProperties( url, info );

	/*
	** Establish the connection.
	*/
	conn = new EdbcConnect( getHost( url ), info, trace,
				DriverManager.getLoginTimeout() );

    }
    catch( EdbcEx ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".connect(): error establishing connection" );
	if ( trace.enabled( 1 ) )  ex.trace( trace );
	throw ex;
    }

    if ( trace.enabled() )  trace.log( title + ".connect(): " + conn );
    return( conn );
} // connect


/*
** Name: setTraceLog
**
** Description:
**	Set the driver trace log.
**
** Input:
**	log	Name of log, NULL to disable.
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
**	28-Mar-01 (gordy)
**	    Initial trace message now done by tracing.
*/

public static void
setTraceLog( String log )
{
    trace.setTraceLog( log );
    return;
}


/*
** Name: setTraceLevel
**
** Description:
**	Set the tracing level for a particular trace ID.
**
** Input:
**	id	Trace ID.
**	level	Tracing level.
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
*/

public static void
setTraceLevel( String id, int level )
{
    trace.setTraceLevel( id, level );
    if ( id.equals( EdbcTrace.edbcTraceID ) )  trace.setTraceLevel( level );
    return;
}


/*
** Name: setTraceLevel
**
** Description:
**	Set the driver tracing level.
**
** Input:
**	level	Tracing level.
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
*/

public static void
setTraceLevel( int level )
{
    setTraceLevel( EdbcTrace.edbcTraceID, level );
    return;
}


/*
** Name: accept
**
** Description:
**	Checks a URL to see if it is handled by this driver.  
**	This driver handles URL's of the form:
**
**	    jdbc:edbc:<sub-name>
**
** Input:
**	url	    Connection target URL.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if URL accepted, false otherwise.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
*/

private boolean
accept( String url )
{
    return( url.regionMatches( true, 0, protocol, 0, protocol.length() ) );
} // accept

/*
** Name: getProperties
**
** Description:
**	Returns the properties extracted as attributes from a
**	URL combined with an existing set of properties.  The
**	URL attributes are only used if the property is not
**	already defined.  This works for iterative building
**	of the properties since the first call to this routine
**	will move the attributes to the property list and
**	subsequent calls will use the property list (which
**	may or may not be changed).
**
** Input:
**	url	    Connection target URL
**	info	    Current property set.
**
** Output:
**	None.
**
** Returns:
**	Properties  Combined property set.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
**	28-Mar-01 (gordy)
**	    Replaced propDB.name with cp_db.
*/

private Properties
getProperties( String url, Properties info )
    throws SQLException
{
    String  attr, name, value;
    int	    start, next, eq_sign;

    if ( info == null )  info = new Properties();

    /*
    ** The database portion of the URL is a special case:
    ** it is extracted and saved as a 'known' property.
    */
    if ( info.getProperty( cp_db ) == null  &&
	 (value = getDatabase( url )) != null )  
	info.put( cp_db, value );
    
    if ( (attr = getAttributes( url )) == null )  return( info );

  loop:	

    for( start = 0; start < attr.length(); start = next + 1 )
    {
	/*
	** Extract next attribute name and value.
	*/
	if ( (next = attr.indexOf( ';', start )) < start )
	    next = attr.length();
	else  if ( next == start )    // Skip empty attribute
	    continue;

	if ( (eq_sign = attr.indexOf( '=', start )) <= start  ||  
	     eq_sign >= next - 1 )
	{
	    if ( trace.enabled( 1 ) )
		trace.write( tr_id + ": Invalid URL - bad attribute syntax '" +
			     attr.substring( start, next ) );
	    throw EdbcEx.get( E_JD0003_BAD_URL );
	}

	name = attr.substring( start, eq_sign );
	value = attr.substring( eq_sign + 1, next );

	/*
	** Translate attribute name to property name.
	** It is also acceptable to use the property
	** name as the attribute name.
	*/
	for( int i = 0; i < attrInfo.length; i++ )
	    if ( name.equalsIgnoreCase( attrInfo[ i ].attrName )  ||
		 name.equalsIgnoreCase( attrInfo[ i ].propName ) )
	    {
		if ( info.getProperty( attrInfo[ i ].propName ) == null )  
		     info.put( attrInfo[ i ].propName, value );
		continue loop;  // Outer loop - next attribute
	    }

	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": Invalid URL - unrecognized attribute '" + 
			 name + "'" );
	throw EdbcEx.get( E_JD0003_BAD_URL );
    }

    return( info );
} // getProperties

/*
** Name: getHost
**
** Description:
**	Parses a URL and returns the host:port portion.  
**	Assumes the URL has been validated by accept().
**	Handles URLs in the following form:
**
**	jdbc:edbc://<host>:<port>[/<db>][;<attr>=<value>]
**
** Input:
**	url	    Connection target URL.
**
** Output:
**	None.
**
** Returns:
**	String	    Host name and port or null.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
*/

private String
getHost( String url )
{
    String  host = null;
    int     start = protocol.length();	// Skip the protocol
    int	    db, attr; 

    /*
    ** Step over '//' prefix (must be present).
    */
    if ( 
	 url.length() > (start + 1)  &&  
	 url.charAt( start++ ) == '/'  &&  
	 url.charAt( start++ ) == '/' 
       )
    {
	/*
	** Host info ends at database (prefixed by '/'),
	** attributes (prefixed by ';') or end of URL.
	*/
	if ( (db = url.indexOf( '/', start )) < start )  db = url.length();
	if ( (attr = url.indexOf( ';', start )) < start )  attr = url.length();
	host = url.substring( start, Math.min( db, attr ) );
    }

    return( host );
} // getHost

/*
** Name: getDatabase
**
** Description:
**	Parses a URL and returns the database identifier.
**	Assumes the URL has been validated by accept().
**	Handles URLs of the following form:
**
**	jdbc:edbc://<host>:<port>[/<db>][;<attr>=<value>]
**
** Input:
**	url	    Connection target URL.
**
** Output:
**	None.
**
** Returns:
**	String	    Database identifier or null.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
*/

private String
getDatabase( String url )
{
    String  database = null;
    int     start = protocol.length();	// Skip the protocol
    int	    db, attr; 

    /*
    ** Skip the host info.
    */
    if ( 
	 url.length() > (start + 1)  &&  
	 url.charAt( start++ ) == '/'  &&  
	 url.charAt( start++ ) == '/' 
       )
    {
	/*
	** Host info ends at database (prefixed by '/'),
	** attributes (prefixed by ';') or end of URL.
	*/
	if ( (db = url.indexOf( '/', start )) < start )  db = url.length();
	if ( (attr = url.indexOf( ';', start )) < start )  attr = url.length();
	start = Math.min( db, attr );
    }

    /*
    ** Step over '/' prefix (must be present).
    */
    if ( url.length() > start  &&  url.charAt( start++ ) == '/' )
    {
	if ( (attr = url.indexOf( ';', start )) < start )  attr = url.length();
	database = url.substring( start, attr );
    }

    return( database );
} // getDatabase

/*
** Name: getAttributes
**
** Description:
**	Parses a URL and returns the attributes.
**	Assumes the URL has been validated by accept().
**	Handles URLs of the following form:
**
**	jdbc:edbc://<host>:<port>[/<db>][;<attr>=<value>]
**
** Input:
**	url	    Connection target URL.
**
** Output:
**	None.
**
** Returns:
**	String	    Attributes or null.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
*/

private String
getAttributes( String url )
{
    int attr, start = protocol.length();
 
    return( ((attr = url.indexOf( ';', start )) >= start) 
	    ? url.substring( attr + 1 ) : null );
} // getAttributes

} // class EdbcDriver
