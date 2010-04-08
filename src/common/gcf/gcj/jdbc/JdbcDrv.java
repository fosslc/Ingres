/*
** Copyright (c) 1999, 2009 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: JdbcDrv.java
**
** Description:
**	Defines class which implements the JDBC Driver interface.
**
**  Classes:
**
**	JdbcDrv
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
**	31-Oct-02 (gordy)
**	    Converted to generic base driver class.
**	26-Dec-02 (gordy)
**	    Pass connection properties to DrvConn.
**	27-Feb-03 (gordy)
**	    Add missing values to DriverPropertyInfo.
**	15-Jul-03 (gordy)
**	    Enhanced configuration support.
**      05-Jan-09 (rajus01) SIR 121238
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to
**            support JDBC 4.0 SQLException hierarchy.
**	 5-May-09 (gordy)
**	    Support multiple host/port list targets.
*/

import	java.util.Properties;
import	java.util.ResourceBundle;
import	java.sql.DriverManager;
import	java.sql.Driver;
import	java.sql.DriverPropertyInfo;
import	java.sql.Connection;
import	java.sql.SQLException;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.util.GcfErr;
import	com.ingres.gcf.util.Config;
import	com.ingres.gcf.util.ConfigEmpty;
import	com.ingres.gcf.util.ConfigKey;
import	com.ingres.gcf.util.ConfigProp;
import	com.ingres.gcf.util.TraceLog;


/*
** Name: JdbcDrv
**
** Description:
**	JDBC driver base class which implements the JDBC Driver 
**	interface.  This driver handles URL's of the form:
**
**	jdbc:<protocol>://<host>:<port>[/<database>][;<attr>=<value>]
**
**	The protocol is defined by the sub-class.  Sub-classes must 
**	implement the following load methods defined by this class to
**	provide driver identity and runtime configuration information.
**
**	    loadConfig		Configuration properties.
**	    loadDriverName	Full name of driver.
**	    loadProtocolID	Protocol accepted by driver.
**	    loadTraceName	Name of driver for tracing.
**	    loadTraceLog	Tracing log.
**
**	Attributes and properties are defined in the DrvConst
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
**  Constants
**
**	driverVendor		Vendor name
**	driverJdbcVersion	Supported JDBC version
**	driverMajorVersion	Internal driver version
**	driverMinorVersion
**	driverPatchVersion
**
**  Abstract Methods:
**
**	loadConfig		Load config properties from sub-class.
**	loadDriverName		Load driver name from sub-class.
**	loadProtocolID		Load protocol ID from sub-class.
**	loadTraceName		Load tracing name from sub-class.
**	loadTraceLog		Load tracing log from sub-class.
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
**  Private Data:
**
**	full_protocol_id    Complete URL protocol.
**	trace		    DriverManager tracing.
**	title		    Class tracing name.
**	tr_id		    Class id for tracing.
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
**	31-Oct-02 (gordy)
**	    Genericized and made abstract as base class for productized
**	    drivers.  Added constants driverVendor, driverJdbcVersion,
**	    driverMajorVersion and driverMinorVersion, sub-class config
**	    fields driverName, traceName and protocol and local field
**	    full_protocol_id.
**	15-Jul-03 (gordy)
**	    Replaced setDriverIdentity() with load methods and removed
**	    associated fields.
**      14-Feb-08 (rajus01) SIR 119917
**          Added support for JDBC driver patch version for patch builds.
*/

public abstract class
JdbcDrv 
    implements	Driver, DrvConst, GcfErr
{

    /*
    ** Driver identifying constants.
    */
    public static final String	driverVendor = DRV_VENDOR_NAME;
    public static final String	driverJdbcVersion = DRV_JDBC_VERSION;
    public static final int	driverMajorVersion = DRV_MAJOR_VERSION;
    public static final int	driverMinorVersion = DRV_MINOR_VERSION;
    public static final int	driverPatchVersion = DRV_PATCH_VERSION;

    /*
    ** Combined protocol identifier to facilitate URL parsing.
    */
    private	String		full_protocol_id = null;

    /*
    ** Tracing.
    */
    private	TraceDM		trace = null;
    private	String		title = "JDBC-Driver";
    
    private static final String	tr_id = "Drv";


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
**	31-Oct-02 (gordy)
**	    Removed DriverManager registration (must be done by sub-class).
*/

static
{
    /*
    ** Load the Locale specific descriptions for connection properties.
    */
    ResourceBundle	rsrc = DrvRsrc.getResource();

    for( int i = 0; i < propInfo.length; i++ )
	try { propInfo[ i ].desc = rsrc.getString( propInfo[ i ].rsrcID ); }
	catch( Exception ignore ){}

} // <class initializer>


/*
** The following abstract mehtods must be implemented by sub-classes
** to provide driver identity and runtime configuration information:
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

protected abstract Config	loadConfig();
protected abstract String	loadDriverName();
protected abstract String	loadProtocolID();
protected abstract String	loadTraceName();
protected abstract TraceLog	loadTraceLog();

 
/*
** Name: JdbcDrv
**
** Description:
**	Class constructor.
**
** Input:
**	None.
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
**	    Configure for productized sub-class.
**	15-Jul-03 (gordy)
**	    Driver now configures identity using load methods.
*/

protected
JdbcDrv()
{
    full_protocol_id = DRV_JDBC_PROTOCOL_ID + ":" + loadProtocolID() + ":";
    trace = new TraceDM( loadTraceLog(), loadTraceName(), DRV_TRACE_ID );
    title = loadTraceName() + "-Driver";
} // JdbcDrv


/*
** Name: toString
**
** Description:
**	Returns the name of the driver.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	Driver name.
**
** History:
**	28-Mar-03 (gordy)
**	    Created.
*/

public String
toString()
{
    return( loadDriverName() );
} // toString


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
	trace.log( title + ".getMajorVersion(): " + driverMajorVersion );
    return( driverMajorVersion );
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
	trace.log( title + ".getMinorVersion(): " + driverMinorVersion );
    return( driverMinorVersion );
} // getMinorVersion

/*
** Name: getPatchVersion
**
** Description:
**	Returns driver patch version.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Patch version for bug fixes.
**
** History:
**	 14-Feb-08 (rajus01)
**	    Created.
*/

public int
getPatchVersion()
{
    if ( trace.enabled() )  
	trace.log( title + ".getPatchVersion(): " + driverPatchVersion );
    return( driverPatchVersion );
} // getPatchVersion

/*
** Name: acceptsURL
**
** Description:
**	Returns an indication that the URL provided is handled by
**	this driver.  This driver handles URL's of the form:
**
**	    jdbc:<protocol>:
**
**	where protocol is a driver configured string.
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
**	prop	    Current property set.
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
**	27-Feb-03 (gordy)
**	    If property value not set, force the default value as required
**	    by the JDBC spec for DriverPropertyInfo.value.  Return choices.
**	15-Jul-03 (gordy)
**	    Enhanced configuration capabilities.
*/

public DriverPropertyInfo[]	
getPropertyInfo( String url, Properties prop )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getPropertyInfo()" );

    if ( ! accept( url ) )  
    {
	if ( trace.enabled() )  
	    trace.log( title + ".getPropertyInfo(): URL not accepted!" );
	throw SqlExFactory.get( ERR_GC4000_BAD_URL );
    }

    Config config = getProperties( url, prop );
    DriverPropertyInfo dp[] = new DriverPropertyInfo[ propInfo.length ];

    for( int i = 0; i < propInfo.length; i++ )
    {
	String value = config.get( propInfo[ i ].name );
	if ( value == null )  value = propInfo[ i ].dflt;
	dp[ i ] = new DriverPropertyInfo( propInfo[ i ].name, value );
	dp[ i ].description = propInfo[ i ].desc;
	dp[ i ].required = propInfo[ i ].req;
	dp[ i ].choices = propInfo[ i ].valid;
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
**	prop	Driver property value pairs.
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
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	26-Dec-02 (gordy)
**	    Pass connection properties to DrvConn.
**	15-Jul-03 (gordy)
**	    Enhanced configuration capabilities.
**	 3-Jul-06 (gordy)
**	    DrvConn now takes general configuration settings rather
**	    than driver properties.
*/

public Connection
connect( String url, Properties prop )
    throws SQLException
{
    DrvConn	conn;
    JdbcConn	jdbc;

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
	Config config = getProperties( url, prop );

	/*
	** Establish server connection.
	*/
	conn = new DrvConn( getHost( url ), loadConfig(), trace, false );
	conn.driverName = loadDriverName();
	conn.protocol = loadProtocolID();

	/*
	** Establish DBMS connection
	*/
	jdbc = new JdbcConn( conn, config, DriverManager.getLoginTimeout() );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".connect(): error establishing connection" );
	if ( trace.enabled( 1 ) )  SqlExFactory.trace( ex, trace );
	throw ex;
    }

    if ( trace.enabled() )  trace.log( title + ".connect(): " + jdbc );
    return( jdbc );
} // connect


/*
** Name: setTraceLog
**
** Description:
**	Set the driver trace log.
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
**	28-Mar-01 (gordy)
**	    Initial trace message now done by tracing.
**	31-Oct-02 (gordy)
**	    Tracing no longer static (single log maintained by tracing class).
*/

public void
setTraceLog( String name )
{
    trace.getTraceLog().setTraceLog( name );
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
**	31-Oct-02 (gordy)
**	    Tracing no longer static (single log maintained by tracing class).
*/

public void
setTraceLevel( String id, int level )
{
    trace.getTraceLog().getTraceLevel( id ).set( level );
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
**	31-Oct-02 (gordy)
**	    Tracing no longer static (single log maintained by tracing class).
*/

public void
setTraceLevel( int level )
{
    trace.setTraceLevel( level );
    return;
}


/*
** Name: accept
**
** Description:
**	Checks a URL to see if it is handled by this driver.  
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
    return( url.regionMatches( true, 0, full_protocol_id, 0, 
					full_protocol_id.length() ) );
} // accept


/*
** Name: getProperties
**
** Description:
**	Returns the properties extracted as attributes from a
**	URL combined with a set of connection properties and
**	the global driver property set.  
**
** Input:
**	url	Connection target URL
**	prop	Connection property set.
**
** Output:
**	None.
**
** Returns:
**	Config	Combined property set.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
**	28-Mar-01 (gordy)
**	    Replaced propDB.name with cp_db.
**	15-Jul-03 (gordy)
**	    Changed return type to support global driver property
**	    sets.  URL attributes now take precedence over other
**	    properties.
*/

private Config
getProperties( String url, Properties prop )
    throws SQLException
{
    Properties	attrs = new Properties();
    Config	config;
    String	attr, name, value;
    int		start, next, eq_sign;
    
    /*
    ** Build up the properties config heirarchy:
    **
    **	Driver global properties (prefixed by 'property')
    **	Driver connection properties
    **	URL properties.
    */
    config = new ConfigKey( DRV_PROPERTY_KEY, loadConfig() );
    if ( prop != null )  config = new ConfigProp( prop, config );
    config = new ConfigProp( attrs, config );

    /*
    ** The database portion of the URL is a special case:
    ** it is extracted and saved as a 'known' property.
    */
    if ( (value = getDatabase( url )) != null )  
	attrs.put( DRV_PROP_DB, value );
    
    if ( (attr = getAttributes( url )) == null )  return( config );

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
	    throw SqlExFactory.get( ERR_GC4000_BAD_URL );
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
		attrs.put( attrInfo[ i ].propName, value );
		continue loop;  // Outer loop - next attribute
	    }

	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": Invalid URL - unrecognized attribute '" + 
			 name + "'" );
	throw SqlExFactory.get( ERR_GC4000_BAD_URL );
    }

    return( config );
} // getProperties


/*
** Name: getHost
**
** Description:
**	Parses a URL and returns the host:port portion.  
**	Assumes the URL has been validated by accept().
**	Handles URLs in the following form:
**
**	jdbc:<protocol>://<host>:<port>[/<db>[;<attr>=<value>]]
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
**	 5-May-09 (gordy)
**	    Attributes are now restricted to path component
**	    to support ';' separated list of targets.
*/

private String
getHost( String url )
{
    /*
    ** Find start and end of host section in URL.
    */
    int start = findHost( url );
    int end = findDb( url, start ); 

    /*
    ** Return null if host section is not found.
    ** Otherwise, skip "//" prefix.
    */
    return( (start < url.length()) ? url.substring( start + 2, end ) : null );
} // getHost


/*
** Name: getDatabase
**
** Description:
**	Parses a URL and returns the database identifier.
**	Assumes the URL has been validated by accept().
**	Handles URLs of the following form:
**
**	jdbc:<protocol>://<host>:<port>[/<db>[;<attr>=<value>]]
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
**	 5-May-09 (gordy)
**	    Attributes must be part of path.
*/

private String
getDatabase( String url )
{
    /*
    ** Find start and end of database section.
    */
    int start = findDb( url, findHost( url ) );
    int end = findAttr( url, start );

    /*
    ** Return null if database section is not found.
    ** Otherwise, skip "/" prefix.
    */
    return( (start < url.length()) ? url.substring( start + 1, end ) : null );
} // getDatabase


/*
** Name: getAttributes
**
** Description:
**	Parses a URL and returns the attributes.
**	Assumes the URL has been validated by accept().
**	Handles URLs of the following form:
**
**	jdbc:<protocol>://<host>:<port>[/<db>[;<attr>=<value>]]
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
**	 5-May-09 (gordy)
**	    Attributes must be part of path.
*/

private String
getAttributes( String url )
{
    /*
    ** Find start of attributes section
    ** (extends to end of URL if present).
    */
    int start = findAttr( url, findDb( url, findHost( url ) ) );
 
    /*
    ** Return null if attributes section is not found.
    ** Otherwise, skip ";" prefix.
    */
    return( (start < url.length()) ? url.substring( start + 1 ) : null );
} // getAttributes


/*
** Name: findHost
**
** Description:
**	Returns the offset of the host target section of a URL.  
**	The host target section follows the protocol ID and is
**	prefixed with "//".  The offset returned is the offset 
**	of the leading "/" or the length of the URL if the host 
**	target section is not found.
**
** Input
**	url	
**
** Output:
**	None.
**
** Returns:
**	int	Offset of host target section in url.
**
** History:
*/

private int
findHost( String url )
{
    /*
    ** Host target follows protocol ID.
    */
    int offset = full_protocol_id.length();

    /*
    ** Host target is prefixed with "//".
    */
    if ( url.length() <= (offset + 1)  ||
	 url.charAt( offset ) != '/'  ||  url.charAt( offset + 1 ) != '/' )
	offset = url.length();

    return( offset );
}


/*
** Name: findDb
**
** Description:
**	Returns the offset of the database section of a URL.
**	The database section follows the host target section
**	and is prefixed with "/".  The offset of the host
**	target section, as returned by findHost(), must be
**	provided.  The offset returned is the offset of the 
**	"/" prefix or the length of the URL if the database 
**	section is not found.
**
** Input:
**	url
**	hostOffset	Offset of host target section.
**
** Output:
**	None.
**
** Returns:
**	int		Offset of database section in url.
**
** Nistory:
*/

private int
findDb( String url, int hostOffset )
{
    /*
    ** Database follows host target and is prefixed by "/".  
    ** Skip over host prefix "//".
    */
    int offset = url.indexOf( '/', hostOffset + 2 );

    return( (offset < 0) ? url.length() : offset );
}


/*
** Name: findAttr
**
** Description:
**	Returns the offset of the attributes section of a URL.
**	The attributes section follows the database section
**	and is prefixed with ";".  The offset of the database
**	section, as returned by findDb(), must be provided.
**	The offset returned is the offset of the ";" prefix
**	or the length of the URL if the attributes section is
**	not found.
**
** Input:
**	url
**	dbOffset	Offset of database section.
**
** Output:
**	None.
**
** Returns:
**	int		Offset of attributes section in url.
**
** History:
*/

private int
findAttr( String url, int dbOffset )
{
    /*
    ** Attributes follow database and are prefixed by ";".
    ** Skip over database prefix "/".
    */
    int offset = url.indexOf( ';', dbOffset + 1 );

    return( (offset < 0) ? url.length() : offset );
}


} // class JdbcDrv
