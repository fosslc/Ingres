/*
** Copyright (c) 2001, 2010 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: JdbcDS.java
**
** Description:
**	Defines class which implements the JDBC DataSource interface.
**
**	The Ingres JDBC driver permits multiple hosts and ports to be
**	specified as connection targets.  Generally, a single host/port 
**	pair is specified:
**
**	    setServerName( "host1" );
**	    setPortName( "port1" );
**
**	The portName may be a comma separated list of ports to be 
**	associated with serverName: 
**
**	    setServerName( "host1" );
**	    setPortName( "port1,port2,port3" );
**
**	Multiple hosts and ports may be specified as a semi-colon
**	separated list entirely within serverName without setting
**	portName or portNumber:
**
**	    setServerName( "host1:port11,port12;host2:port21,port22" );
**
**	Another possibility is to end serverName with a host name
**	and place the associated port list in portName:
**
**	    setServerName( "host1:port11,port12;host2" );
**	    setPortName( "port21,port22" );
**
**  Classes
**
**	JdbcDS
**
** History:
**	26-Jan-01 (gordy)
**	    Created.
**	18-Apr-01 (gordy)
**	    Made fields protected and added getHost() method for sub-
**	    classes which need their own connections independent of
**	    the standard connection described by the property values.
**	31-Oct-02 (gordy)
**	    Converted to generic base data source class.
**	26-Dec-02 (gordy)
**	    Added new properties cursorMode, loginType and charEncode.
**	18-Feb-03 (gordy)
**	    Remove dataSourceName which is used by data sources which
**	    are covers for other data sources (connection pool managers).
**	    Renamed loginType property to vnodeUsage.  Passwords are now
**	    encoded.
**	15-Jul-03 (gordy)
**	    Enhanced configuration support.  Made serializable
**	23-Jul-07 (gordy)
**	    Added connection property for date alias.
**      05-Jan-09 (rajus01) SIR 121238
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to
**            support JDBC 4.0 SQLException hierarchy.
**	 5-May-09 (gordy)
**	    Support multiple host/port list targets.
**      11-Feb-10 (rajus01) Bug 123277
**          Added connection property 'send_ingres_dates' to allow the
**          driver to send the date/time/timestamp values as INGRESDATE
**          datatype instead of TS_W_TZ (ANSI TIMESTAMP WITH TIMEZONE).
**	30-Jul-10 (gordy)
**	    Added connection property 'send_integer_booleans' to allow the
**	    driver to send boolean values as TINYINT datatype.
*/


import	java.util.Properties;
import	java.io.ObjectInputStream;
import	java.io.PrintWriter;
import	java.io.Serializable;
import	java.io.IOException;
import	java.sql.Connection;
import	java.sql.SQLException;
import	javax.sql.DataSource;
import	javax.naming.Reference;
import	javax.naming.RefAddr;
import	javax.naming.BinaryRefAddr;
import	javax.naming.StringRefAddr;
import	com.ingres.gcf.util.Config;
import	com.ingres.gcf.util.ConfigKey;
import	com.ingres.gcf.util.ConfigProp;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.util.TraceLog;
import	com.ingres.gcf.util.XaXid;


/*
** Name: JdbcDS
**
** Description:
**	JDBC driver base class which implements the JDBC DataSource
**	interface.
**
**	The following data source properties are supported:
**
**	    description		DataSource description
**	    serverName		Target machine name.
**	    portNumber		Numeric port ID.
**	    portName		Symbolic port ID.
**	    databaseName	Database.
**	    user		Username for connection
**	    password		Password for connection.
**	    roleName		Role used in DBMS.
**	    groupName		Group used in DBMS.
**	    dbmsUser		Username used in DBMS.
**	    dbmsPassword	Password used in DBMS.
**	    connectionPool	Server connection pooling ('off' or 'on').
**	    autocommitMode	Autocommit mode ('dbms', 'single', 'multi').
**	    selectLoops		Use select loops not cursors ('off' or 'on').
**	    cursorMode		Cursor mode ('dbms', 'update', 'readonly').
**	    vnodeUsage		VNODE usage ('login', 'connect').
**	    charEncode		Character encoding.
**	    timeZone		Timezone.
**	    decimalChar		Decimal character.
**	    dateFormat		Date format.
**	    moneyFormat		Money format.
**	    moneyPrecision	Money precision.
**	    dateAlias		Date alias.
**	    sendIngresDates	Send Ingres Dates.
**	    sendIntegerBooleans	Send booleans as integer.
**
**	Sub-classes must implement the following load methods defined by 
**	this class to provide driver identity and runtime configuration 
**	information.
**
**	    loadConfig		Configuration properties.
**	    loadDriverName	Full name of driver.
**	    loadProtocolID	Protocol accepted by driver.
**	    loadTraceName	Name of driver for tracing.
**	    loadTraceLog	Tracing log.
**
**  Constants
**
**	driverVendor		Vendor name
**	driverJdbcVersion	Supported JDBC version
**	driverMajorVersion	Internal driver version
**	driverMinorVersion
**	driverPatchVersion
**
**  Abstract Methods
**
**	loadConfig		Load config properties from sub-class.
**	loadDriverName		Load driver name from sub-class.
**	loadProtocolID		Load protocol ID from sub-class.
**	loadTraceName		Load tracing name from sub-class.
**	loadTraceLog		Load tracing log from sub-class.
**
**  Interface Methods:
**
**	getConnection		Open a new connection.
**	getLoginTimeout		Returns login timeout value.
**	setLoginTimeout		Set login timeout value.
**	getLogWriter		Returns tracing output writer.
**	setLogWriter		Set tracing output writer.
**
**  Public Methods:
**
**	getDescription		Property getter/setter methods
**	setDescription
**	getServerName
**	setServername
**	getPortNumber
**	setPortNumber
**	getPortName
**	setPortName
**	getDatabaseName
**	setDatabaseName
**	getUser
**	setUser
**	getPassword
**	setPassword
**	getRoleName
**	setRoleName
**	getGroupName
**	setGroupName
**	getDbmsUser
**	setDbmsUser
**	getDbmsPassword
**	setDbmsPassword
**	getConnectionPool
**	setConnectionPool
**	getAutocommitMode
**	setAutocommitMode
**	getSelectLoops
**	setSelectLoops
**	getCursorMode
**	setCursorMode
**	getVnodeUsage
**	setVnodeUsage
**	getCharEncode
**	setCharEncode
**	getTimeZone
**	setTimeZone
**	getDecimalChar
**	setDecimalChar
**	getDateFormat
**	setDateFormat
**	getMoneyFormat
**	setMoneyFormat
**	getMoneyPrecision
**	setMoneyPrecision
**	getDateAlias
**	setDateAlias
**
**  Protected Data:
**
**	trace			DataSource tracing.
**	title			Class title for tracing.
**	tr_id			Class ID for tracing.
**	inst_id			Instance ID.
**
**  Protected Methods:
**
**	connect			Establish DBMS connection
**	getHost			Returns target host identifier.
**	initReference		Init a JNDI Reference.
**	initInstance		Init JdbcDS from JNDI Reference.
**	encodeIntRef		Add integer reference.
**	decodeIntRef		Retrieve integer reference.
**
**  Private Data:
**
**	DFLT_PWD_KEY		Encoding key when dynamic key not available.
**	conn_trace		Connection tracing.
**	inst_count		Instance counter.
**
**	description		Property values
**	serverName
**	portNumber
**	portName
**	databaseName
**	user
**	password
**	roleName
**	groupName
**	dbmsUser
**	dbmsPassword
**	connectionPool
**	autocommitMode
**	selectLoops
**	cursorMode
**	vnodeUsage
**	charEncode
**	timeZone
**	decimalChar
**	dateFormat
**	moneyFormat
**	moneyPrecision
**	dateAlias
**	sendIngresDates
**	sendIntegerBooleans
**
**	timeout			Connection timeout.
**
**  Private Methods:
**
**	initialize		Initialize transient state.
**	readObject		Deserialize data source.
**	encode			Add encode password reference.
**	decode			Retrieve encoded password reference.
**	getProperties
**
** History:
**	26-Jan-01 (gordy)
**	    Created.
**	18-Apr-01 (gordy)
**	    Made fields protected and added getHost() method for sub-
**	    classes which need their own connections independent of
**	    the standard connection described by the property values.
**	31-Oct-02 (gordy)
**	    Genericized and made abstract as base class for productized
**	    drivers.  Added constants driverVendor, driverJdbcVersion,
**	    driverMajorVersion and driverMinorVersion, sub-class config
**	    fields driverName, traceName and protocol and local fields
**	    log, inst_id and inst_count.  Removed getReference() method
**	    which must be implemented by sub-class.  Added connect(), 
**	    getProperties(), and initInstance() methods.
**	26-Dec-02 (gordy)
**	    Added new properties cursorMode, loginType and charEncode.
**	18-Feb-03 (gordy)
**	    Remove dataSourceName.  Add help methods encodeIntRef(), 
**	    decodeIntRef(), encodePwdRef(), decodePwdRef() and default
**	    encoding key DFLT_PWD_KEY.  Renamed loginType to vnodeUsage.
**	15-Jul-03 (gordy)
**	    Replaced setDriverIdentity() with load methods and removed
**	    associated fields.  Passwords are now stored encoded.  Added
**	    properties timeZone, decimalChar, dateFormat, moneyFormat 
**	    and moneyPrecision.  Marked non-property fields as transient
**	    and added initialize() and readObject to support serialization.
**	23-Jul-07 (gordy)
**	    Added property dateAlias, methods getDateAlias(), setDateAlias().
**	14-Feb-08 (rajus01) SIR 119917
**	    Added support for JDBC driver patch version for patch builds.
**	30-Jul-10 (gordy)
**	    Added property sendIntegerBooleans.
*/

public abstract class
JdbcDS
    implements	DataSource, Serializable, DrvConst
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
    ** Tracing information (not serialized).
    */
    protected transient TraceDS	trace = null;		// DataSource tracing.
    protected transient String	title = "JDBC-DataSource";
    protected transient String	tr_id = "DSrc";
    protected transient int	inst_id = 0;
    
    private transient TraceDS	conn_trace = null;	// Driver tracing.
    private static int		inst_count = 0;		// Instance counter

    private static final String	DFLT_PWD_KEY = "password";
    
    /*
    ** Properties (serialized state).
    */
    private	String		description = null;
    private	String		serverName = null;
    private	int		portNumber = 0;
    private	String		portName = null;
    private	String		databaseName = null;
    private	String		user = null;
    private	byte[]		password = null;
    private	String		roleName = null;
    private	String		groupName = null;
    private	String		dbmsUser = null;
    private	byte[]		dbmsPassword = null;
    private	String		connectionPool = null;
    private	String		autocommitMode = null;
    private	String		selectLoops = null;
    private	String		cursorMode = null;
    private	String		vnodeUsage = null;
    private	String		charEncode = null;
    private	String		timeZone = null;
    private	char		decimalChar = 0;
    private	String		dateFormat = null;
    private	String		moneyFormat = null;
    private	int		moneyPrecision = -1;
    private	String		dateAlias = null;
    private	String		sendIngresDates = null;
    private	String		sendIntegerBooleans = null;

    private	int		timeout = 0;		// Connection timeout


/*
** The following abstract methods must be implemented by sub-classes
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
** Name: JdbcDS
**
** Description:
**	Class constructor.  Define the identity of this JDBC
**	DataSource and driver.
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
**	26-Jan-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Configure for productized sub-class.
**	15-Jul-03 (gordy)
**	    Moved initialization to separate method to support serialization.
*/

protected
JdbcDS() 
{
    initialize();	// Initialize transient data.
} // JdbcDS


/*
** Name: initialize
**
** Description:
**	Initializes the transient data.
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
**	15-Jul-03 (gordy)
**	    Extracted from constructor.
*/

private void
initialize()
{
    trace = new TraceDS( loadTraceLog(), loadTraceName(), DSRC_TRACE_ID );
    conn_trace = new TraceDS( loadTraceLog(), loadTraceName(), DRV_TRACE_ID );
    inst_id = inst_count++;
    title = loadTraceName() + "-DataSource[" + inst_id + "]";
    tr_id += "[" + inst_id + "]";
    return;
} // initialize


/*
** Property getter/setter methods.
**
** Each supported property has methods to set and retrieve
** the property value.
**
** History:
**	15-Jul-03 (gordy)
**	    Passwords now stored encoded.  Added timeZone, decimalChar,
**	    dateFormat, moneyFormat and moneyPrecision.
*/

public String 
getDescription() 
{ return( description ); }

public void
setDescription( String description )
{
    if ( trace.enabled() )  trace.log( title + ": " + description );
    this.description = description; 
}

public String 
getServerName() 
{ return( serverName ); }

public void
setServerName( String serverName )
{ this.serverName = serverName; }

public int 
getPortNumber() 
{ return( portNumber ); }

public void
setPortNumber( int portNumber )
{ this.portNumber = portNumber; }

public String 
getPortName() 
{ return( portName ); }

public void
setPortName( String portName )
{ this.portName = portName; }

public String 
getDatabaseName() 
{ return( databaseName ); }

public void
setDatabaseName( String databaseName )
{ this.databaseName = databaseName; }

public String 
getUser() 
{ return( user ); }

public void
setUser( String user )
{ 
    /*
    ** Need to re-encode password with new key.
    */
    String password = decode( this.password, this.user );
    this.password = encode( password, user );
    this.user = user;
}

public String 
getPassword() 
{ return( "*****" ); }

public void
setPassword( String password )
{ this.password = encode( password, user ); }

public String 
getDbmsUser() 
{ return( dbmsUser ); }

public void
setDbmsUser( String dbmsUser )
{ 
    /*
    ** Need to re-encode password with new key.
    */
    String dbmsPassword = decode( this.dbmsPassword, this.dbmsUser );
    this.dbmsPassword = encode( dbmsPassword, dbmsUser );
    this.dbmsUser = dbmsUser; 
}

public String 
getDbmsPassword() 
{ return( "*****" ); }

public void
setDbmsPassword( String dbmsPassword )
{ this.dbmsPassword = encode( dbmsPassword, dbmsUser ); }

public String 
getRoleName() 
{ return( roleName ); }

public void
setRoleName( String roleName )
{ this.roleName = roleName; }

public String 
getGroupName() 
{ return( groupName ); }

public void
setGroupName( String groupName )
{ this.groupName = groupName; }

public String 
getConnectionPool() 
{ return( connectionPool ); }

public void
setConnectionPool( String connectionPool )
{ this.connectionPool = connectionPool; }

public String 
getAutocommitMode() 
{ return( autocommitMode ); }

public void
setAutocommitMode( String autocommitMode )
{ this.autocommitMode = autocommitMode; }

public String 
getSelectLoops() 
{ return( selectLoops ); }

public void
setSelectLoops( String selectLoops )
{ this.selectLoops = selectLoops; }

public String 
getCursorMode() 
{ return( cursorMode ); }

public void
setCursorMode( String cursorMode )
{ this.cursorMode = cursorMode; }

public String 
getVnodeUsage() 
{ return( vnodeUsage ); }

public void
setVnodeUsage( String vnodeUsage )
{ this.vnodeUsage = vnodeUsage; }

public String 
getCharEncode() 
{ return( charEncode ); }

public void
setCharEncode( String charEncode )
{ this.charEncode = charEncode; }

public String
getTimeZone()
{ return( timeZone ); }

public void
setTimeZone( String timeZone )
{ this.timeZone = timeZone; }

public String
getDecimalChar()
{ return( (decimalChar == 0) ? null : String.valueOf( decimalChar ) ); }

public void
setDecimalChar( String dc )
{ this.decimalChar = (dc == null  ||  dc.length() == 0) ? 0 : dc.charAt(0); }

public String
getDateFormat()
{ return( dateFormat ); }

public void
setDateFormat( String dateFormat )
{ this.dateFormat = dateFormat; }

public String
getMoneyFormat()
{ return( moneyFormat ); }

public void
setMoneyFormat( String moneyFormat )
{ this.moneyFormat = moneyFormat; }

public int
getMoneyPrecision()
{ return( moneyPrecision ); }

public void
setMoneyPrecision( int moneyPrecision )
{ this.moneyPrecision = moneyPrecision; }

public String
getDateAlias()
{ return( dateAlias ); }

public void
setDateAlias( String dateAlias )
{ this.dateAlias = dateAlias; }

public String
getSendIngresDates()
{ return( sendIngresDates ); }

public void
setSendIngresDates( String sendIngresDates )
{ this.sendIngresDates = sendIngresDates; }

public String
getSendIntegerBooleans()
{ return( sendIntegerBooleans ); }

public void
setSendIntegerBooleans( String sendIntegerBooleans )
{ this.sendIntegerBooleans = sendIntegerBooleans; }


/*
** Name: getConnection
**
** Description:
**	Open a new connection and return a JDBC Connection object.  
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Connection  A new connection object.
**
** History:
**	26-Jan-01 (gordy)
**	    Created.
*/

public Connection
getConnection()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getConnection()" );
    Connection conn = connect( (String)null, (String)null );
    if ( trace.enabled() )  trace.log( title + ".getConnection(): " + conn ); 
    return( conn );
} // getConnection


/*
** Name: getConnection
**
** Description:
**	Open a new connection, allocate and return a JDBC Connection
**	object.  
**
** Input:
**	username    User ID.
**	password    Password
**
** Output:
**	None.
**
** Returns:
**	Connection  A new connection object.
**
** History:
**	26-Jan-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Property processing extracted to getProperties() and
**	    connection establishment extracted to connect().
*/

public Connection
getConnection( String user, String password )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getConnection('" + user + "','*****')" );
    Connection conn = connect( user, password );
    if ( trace.enabled() )  trace.log( title + ".getConnection(): " + conn ); 
    return( conn );
} // getConnection



/*
** Name: getLoginTimeout
**
** Description:
**	Returns the login timeout value in seconds.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Timeout in seconds.
**
** History:
**	26-Jan-01 (gordy)
**	    Created.
*/

public int
getLoginTimeout()
    throws SQLException
{
    return( timeout );
} // getLoginTimeout


/*
** Name: setLoginTimeout
**
** Description:
**	Set the login timeout value in seconds.
**
** Input:
**	seconds	    Timeout value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	26-Jan-01 (gordy)
**	    Created.
*/

public void
setLoginTimeout( int seconds )
    throws SQLException
{
    timeout = seconds;
    return;
} // setLoginTimeout


/*
** Name: getLogWriter
**
** Description:
**	Returns the DataSource trace output writer.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	PrintWriter	Tracing output writer.
**
** History:
**	26-Jan-01 (gordy)
**	    Created.
*/

public PrintWriter
getLogWriter()
    throws SQLException
{
    return( trace.getWriter() );
} // getLogWriter


/*
** Name: setLogWriter
**
** Description:
**	Set the DataSource trace output writer.
**
** Input:
**	out	Tracing output writer.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	26-Jan-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Since a data source trace log may be used by many data sources,
**	    initial driver trace no longer written.
*/

public void
setLogWriter( PrintWriter out )
    throws SQLException
{
    trace.setWriter( out );

    if ( conn_trace.getWriter() != out )
    {
	/*
	** Since the current connection tracing may be used 
	** by a connection, a new trace object is created
	** instead of just setting the trace writer.
	*/
	conn_trace = new TraceDS( loadTraceLog(), 
				  loadTraceName(), DRV_TRACE_ID );
	conn_trace.setWriter( out );
    }
    return;
} // setLogWriter


/*
** Name: connect
**
** Description:
**	Establish a DBMS Connection.  The DataSource user ID
**	and password properties may be overriden by providing
**	alternate values (otherwise provide NULL).
**
** Input:
**	user	    User ID, may be NULL.
**	password    Password, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	JdbcConn    JDBC driver connection.
**
** History:
**	26-Jan-01 (gordy)
**	    Created.
**	18-Apr-01 (gordy)
**	    Extracted host:port formatting to getHost() method.
**	31-Oct-02 (gordy)
**	    Extracted from getConnection().
**	26-Dec-02 (gordy)
**	    Pass connection properties to DrvConn.
**	15-Jul-03 (gordy)
**	    Enhanced configuration capabilities.
**	20-Jul-06 (gordy)
**	    DrvConn now takes general configuration settings rather
**	    than driver properties.
*/

protected JdbcConn
connect( String user, String password )
    throws SQLException
{
    DrvConn	conn;
    Config	config = loadConfig();
    Config	prop;
    String	host = getHost();
    
    if ( trace.enabled( 2 ) )  trace.write( tr_id + ": connect to " + host );
    
    /*
    ** Build up the properties config heirarchy:
    **
    **	Driver global properties (prefixed by 'property')
    **	DataSource connection properties
    */
    prop = new ConfigKey( DRV_PROPERTY_KEY, config );
    prop = new ConfigProp( getProperties( user, password ), prop );

    /*
    ** Establish server connection.
    */
    conn = new DrvConn( host, config, conn_trace, false );
    conn.driverName = loadDriverName();
    conn.protocol = loadProtocolID();

    /*
    ** Establish DBMS connection
    */
    return( new JdbcConn( conn, prop, timeout ) );
} // connect


/*
** Name: connect
**
** Description:
**	Establish a DTM Connection.  If XID is provided, the
**	distributed transaction will be assigned to the connection.
**
** Input:
**	host	    Host name and port: <host>:<port>
**	database    Target database.
**	xid	    XA XID, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	JdbcConn    JDBC driver connection.
**
** History:
**	31-Oct-02 (gordy)
**	    Created.
**	26-Dec-02 (gordy)
**	    Pass connection properties to DrvConn.
**	15-Jul-03 (gordy)
**	    Enhanced configuration capabilities.
**	20-Jul-06 (gordy)
**	    DrvConn now takes general configuration settings rather
**	    than driver properties.
*/

protected JdbcConn
connect( String host, String database, XaXid xid )
    throws SQLException
{
    if ( trace.enabled( 2 ) )  
	trace.write( tr_id + ": connecting (DTMC) to " + host );

    /*
    ** For DTMC connections we only set the database property.
    */
    Properties prop = new Properties();
    prop.setProperty( DRV_PROP_DB, database );

    if ( trace.enabled( 3 ) )
    {
	trace.write( "     " + DRV_PROP_DB + ": " + database );
	if ( xid != null )  trace.write( "     xid: " + xid );
    }

    /*
    ** Establish server connection.
    */
    DrvConn conn = new DrvConn( host, loadConfig(), conn_trace, true );
    conn.driverName = loadDriverName();
    conn.protocol = loadProtocolID();

    /*
    ** Establish DBMS connection
    */
    return( new JdbcConn( conn, new ConfigProp( prop ), xid ) );
}


/*
** Name: getHost
**
** Description:
**	Returns the target host identifier composed of the
**	machine (serverName) and port (portName or portNumber)
**	formatted as needed for the JDBC driver.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	Host ID.
**
** History:
**	18-Apr-01 (gordy)
**	    Created.
**	 5-May-09 (gordy)
**	    If portName and portNumber have not been set, return
**	    just the serverName.  This allows multiple hosts and
**	    port lists to be specified.
*/

protected String
getHost()
{
    String host; 
	
    if ( portName != null )  
	host = serverName + ':' + portName;
    else  if ( portNumber > 0 )
	host = serverName + ':' + portNumber;
    else
	host = serverName;

    return( host );
} // getHost


/*
** Name: initReference
**
** Description:
**	Initialize a Reference object to represent this JdbcDS.
**
** Input:
**	None.
**
** Output:
**	ref	JNDI Reference.
**
** Returns:
**	void.
**
** History:
**	26-Jan-01 (gordy)
**	    Created.
**	26-Dec-02 (gordy)
**	    Added new properties cursorMode, loginType and charEncode.
**	18-Feb-03 (gordy)
**	    Remove dataSourceName.  Added vnodeUsage property.  Extract 
**	    integer/binary handling.  Passwords are encoded.
**	15-Jul-03 (gordy)
**	    Passwords now stored internally encoded, so can save 
**	    directly without encoding.  Added timeZone, decimalChar, 
**	    dateFormat, moneyFormat and moneyPrecision.
**	23-Jul-07 (gordy)
**	    Added property dateAlias.
**	30-Jul-10 (gordy)
**	    Added property sendIntegerBooleans.
*/

protected void
initReference( Reference ref )
{
    if ( description != null )
	ref.add( new StringRefAddr( "description", description ) );

    if ( serverName != null )
	ref.add( new StringRefAddr( "serverName", serverName ) );

    if ( portName != null )
	ref.add( new StringRefAddr( "portName", portName ) );

    if ( portNumber > 0 )  
	encodeIntRef( ref, "portNumber", portNumber );

    if ( databaseName != null )
	ref.add( new StringRefAddr( "databaseName", databaseName ) );

    if ( user != null )
	ref.add( new StringRefAddr( "user", user ) );
    
    if ( password != null )	// Encoded storage indicated by 'v1' extension
        ref.add( new BinaryRefAddr( "password.v1", password ) );

    if ( dbmsUser != null )
	ref.add( new StringRefAddr( "dbmsUser", dbmsUser ) );
    
    if ( dbmsPassword != null )	// Encoded storage indicated by 'v1' extension
        ref.add( new BinaryRefAddr( "dbmsPassword.v1", dbmsPassword ) );

    if ( roleName != null )
	ref.add( new StringRefAddr( "roleName", roleName ) );

    if ( groupName != null )
	ref.add( new StringRefAddr( "groupName", groupName ) );

    if ( connectionPool != null )
	ref.add( new StringRefAddr( "connectionPool", connectionPool ) );

    if ( autocommitMode != null )
	ref.add( new StringRefAddr( "autocommitMode", autocommitMode ) );

    if ( selectLoops != null )
	ref.add( new StringRefAddr( "selectLoops", selectLoops ) );

    if ( cursorMode != null )
	ref.add( new StringRefAddr( "cursorMode", cursorMode ) );

    if ( vnodeUsage != null )
	ref.add( new StringRefAddr( "vnodeUsage", vnodeUsage ) );

    if ( charEncode != null )
	ref.add( new StringRefAddr( "charEncode", charEncode ) );

    if ( timeZone != null )
	ref.add( new StringRefAddr( "timeZone", timeZone ) );
    
    if ( decimalChar != 0 )
	ref.add( new StringRefAddr( "decimalChar", 
				    String.valueOf( decimalChar ) ) );
    
    if ( dateFormat != null )
	ref.add( new StringRefAddr( "dateFormat", dateFormat ) );
    
    if ( moneyFormat != null )
	ref.add( new StringRefAddr( "moneyFormat", moneyFormat ) );
    
    if ( moneyPrecision >= 0 )
	encodeIntRef( ref, "moneyPrecision", moneyPrecision );
    
    if ( dateAlias != null )
	ref.add( new StringRefAddr( "dateAlias", dateAlias ) );

    if ( sendIngresDates != null )
	ref.add( new StringRefAddr( "sendIngresDates", sendIngresDates ) );

    if ( sendIntegerBooleans != null )
	ref.add( new StringRefAddr( "sendIntegerBooleans", 
				    sendIntegerBooleans ) );

    return;
} // initReference


/*
** Name: initInstance
**
** Description:
**	Initialize this JdbcDS object from a JNDI reference.
**
** Input:
**	ref	JNDI Reference.
**
** Output:
**	None.	
**
** Returns:
**	void.
**
** History:
**	26-Jan-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Moved to this class from class factory.
**	26-Dec-02 (gordy)
**	    Added new properties cursorMode, loginType and charEncode.
**	18-Feb-03 (gordy)
**	    Remove dataSourceName.  Added vnodeUsage property.  Extract 
**	    integer/binary handling.  Passwords are encoded.
**	15-Jul-03 (gordy)
**	    Passwords are now stored internally encoded, so simply 
**	    need to save binary values (string values must be decoded).  
**	    Added timeZone, decimalChar, dateFormat, moneyFormat and
**	    moneyPrecision.
**	23-Jul-07 (gordy)
**	    Added property dateAlias.
**	30-Jul-10 (gordy)
**	    Added property sendIntegerBooleans.
*/

protected void
initInstance( Reference ref )
{
    RefAddr ra;

    if ( (ra = ref.get( "description" )) != null )
	description = (String)((StringRefAddr)ra).getContent();

    if ( (ra = ref.get( "serverName" )) != null )
	serverName = (String)((StringRefAddr)ra).getContent();

    if ( (ra = ref.get( "portName" )) != null )
	portName = (String)((StringRefAddr)ra).getContent();

    if ( (ra = ref.get( "portNumber" )) != null )
	portNumber = decodeIntRef( ra );

    if ( (ra = ref.get( "databaseName" )) != null )
	databaseName = (String)((StringRefAddr)ra).getContent();

    if ( (ra = ref.get( "user" )) != null )
	user = (String)((StringRefAddr)ra).getContent();
    
    if ( (ra = ref.get( "password.v1" )) != null )
	password = (byte [])((BinaryRefAddr)ra).getContent();
    else  if ( (ra = ref.get( "password" )) != null )  
	password = encode( (String)((StringRefAddr)ra).getContent(), user );

    if ( (ra = ref.get( "dbmsUser" )) != null )
	dbmsUser = (String)((StringRefAddr)ra).getContent();
    
    if ( (ra = ref.get( "dbmsPassword.v1" )) != null )
	dbmsPassword = (byte [])((BinaryRefAddr)ra).getContent();
    else  if ( (ra = ref.get( "dbmsPassword" )) != null )  
	dbmsPassword = encode( (String)((StringRefAddr)ra).getContent(), 
			       dbmsUser );

    if ( (ra = ref.get( "roleName" )) != null )
	roleName = (String)((StringRefAddr)ra).getContent();

    if ( (ra = ref.get( "groupName" )) != null )
	groupName = (String)((StringRefAddr)ra).getContent();

    if ( (ra = ref.get( "connectionPool" )) != null )
	connectionPool = (String)((StringRefAddr)ra).getContent();

    if ( (ra = ref.get( "autocommitMode" )) != null )
	autocommitMode = (String)((StringRefAddr)ra).getContent();

    if ( (ra = ref.get( "selectLoops" )) != null )
	selectLoops = (String)((StringRefAddr)ra).getContent();

    if ( (ra = ref.get( "cursorMode" )) != null )
	cursorMode = (String)((StringRefAddr)ra).getContent();

    if ( (ra = ref.get( "vnodeUsage" )) != null )
	vnodeUsage = (String)((StringRefAddr)ra).getContent();

    if ( (ra = ref.get( "charEncode" )) != null )
	charEncode = (String)((StringRefAddr)ra).getContent();

    if ( (ra = ref.get( "timeZone" )) != null )
	timeZone = (String)((StringRefAddr)ra).getContent();
    
    if ( (ra = ref.get( "decimalChar" )) != null )
	decimalChar = ((String)((StringRefAddr)ra).getContent()).charAt(0);
    
    if ( (ra = ref.get( "dateFormat" )) != null )
	dateFormat = (String)((StringRefAddr)ra).getContent();
    
    if ( (ra = ref.get( "moneyFormat" )) != null )
	moneyFormat = (String)((StringRefAddr)ra).getContent();
    
    if ( (ra = ref.get( "moneyPrecision" )) != null )
	moneyPrecision = decodeIntRef( ra );
    
    if ( (ra = ref.get( "dateAlias" )) != null )
	dateAlias = (String)((StringRefAddr)ra).getContent();

    if ( (ra = ref.get( "sendIngresDates" )) != null )
	sendIngresDates = (String)((StringRefAddr)ra).getContent();

    if ( (ra = ref.get( "sendIntegerBooleans" )) != null )
	sendIntegerBooleans = (String)((StringRefAddr)ra).getContent();

    return;
} // initInstance


/*
** Name: encodeIntRef
**
** Description:
**	Creates a binary reference for an integer property value.
**
** Input:
**	addr	Property ID.
**	value	Property value.
**
** Output:
**	ref	JNDI Reference.
**
** Returns:
**	void.
**
** History:
**	18-Feb-03 (gordy)
**	    Extracted and generalized from initReference().
*/

protected void
encodeIntRef( Reference ref, String addr, int value )
{
    byte ival[] = new byte[ 4 ];

    ival[0] = (byte)value;
    ival[1] = (byte)(value >> 8);
    ival[2] = (byte)(value >> 16);
    ival[3] = (byte)(value >> 24);

    ref.add( new BinaryRefAddr( addr, ival ) );
    return;
} // encodeIntRef


/*
** Name: decodeIntRef
**
** Description:
**	Returns the integer value for a binary property reference.
**
** Input:
**	ra	Reference Address.
**
** Output:
**	None.
**
** Returns:
**	int	Property value.
**
** History:
**	18-Feb-03 (gordy)
**	    Extracted and generalized from initInstance().
*/

protected int
decodeIntRef( RefAddr ra )
{
    byte ival[] = (byte [])((BinaryRefAddr)ra).getContent();
    
    return( (((int)ival[3] << 24) & 0xff000000) | 
	    (((int)ival[2] << 16) & 0x00ff0000) | 
	    (((int)ival[1] << 8)  & 0x0000ff00) | 
	    ( (int)ival[0]        & 0x000000ff) );
} // decodeIntRef


/*
** Name: readObject
**
** Description:
**	This method is called to deserialize an object of this class.
**	The serialized state is produced by the default serialization
**	methods, so we call the default deserialization method.  Then
**	the transient initialization method is called to enable the
**	complete runtime state.
**
** Input:
**	in	Serialized input stream.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	15-Jul-03 (gordy)
**	    Created.
*/

private void
readObject( ObjectInputStream in )
    throws IOException, ClassNotFoundException
{
    in.defaultReadObject();	// Read serialized state.
    initialize();		// Initialize transient data.
    return;
} // readObject


/*
** Name: encode
**
** Description:
**	Encoded property value with key.
**
** Input:
**	value	Property value.
**	key	Encoding key.
**
** Output:
**	byte[]	Encoded value.
**
** Returns:
**	void.
**
** History:
**	18-Feb-03 (gordy)
**	    Created
**	15-Jul-03 (gordy)
**	    Changed to simply encode values.
*/

private byte[]
encode( String value, String key )
{
    byte    kval[], bval[] = null;
    
    if ( value == null  ||  value.length() == 0 )  
	return( null );	// Nothing to do!
    
    /*
    ** Provide default key.
    */
    if ( key == null  ||  key.length() == 0 )  
	key = DFLT_PWD_KEY;
    
    try
    {
        /*
        ** Convert key and value to binary format.
        */
        kval = key.getBytes("UTF-8");
        bval = value.getBytes("UTF-8");

        /*
        ** Encode password using key.
        */
        for( int i = 0, j = 0; i < bval.length; i++ )
        {
    	    bval[ i ] ^= kval[ j ];
    	    if ( ++j >= kval.length )  j = 0;
        }
    }
    catch( Exception ignore ) {}

    return( bval );
} // encode


/*
** Name: decode
**
** Description:
**	Decode property value with key.
**
** Input:
**	value	Property value
**	key	Encoding key.
**
** Output:
**	None.
**
** Returns:
**	String	Decoded value.
**
** History:
**	18-Feb-03 (gordy)
**	    Created.
**	15-Jul-03 (gordy)
**	    Changed to simply decode values.
*/

private String
decode( byte value[], String key )
{
    String	str = null;
    byte	kval[], bval[];
    
    if ( value == null  ||  value.length == 0 )  
	return( null );	// Nothing to do!
    
    /*
    ** Provide default key.
    */
    if ( key == null  ||  key.length() == 0 )  
	key = DFLT_PWD_KEY;
    
    try
    {
        /*
        ** Convert key to binary and copy value to
	** be decoded to temporary storage.
        */
        kval = key.getBytes("UTF-8");
	bval = new byte[ value.length ];
	System.arraycopy( value, 0, bval, 0, value.length );
	
        /*
        ** Decode value using key.
        */
        for( int i = 0, j = 0; i < bval.length; i++ )
        {
	    bval[ i ] ^= kval[ j ];
	    if ( ++j >= kval.length )  j = 0;
        }

        /*
        ** Convert value back to Unicode.
        */
        str = new String( bval, "UTF-8" );
    }
    catch( Exception ignore ) {}

    return( str );
} // decode


/*
** Name: getProperties
**
** Description:
**	Load DataSource properties into driver property list.
**
** Input:
**	user	    User ID, may be NULL.
**	password    Password, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	Properties  Driver property list.
**
** History:
**	31-Oct-02 (gordy)
**	    Extracted from getConnection().
**	18-Feb-03 (gordy)
**	    Added properties selectLoops, vnodeUsage and charEncode.
**	15-Jul-03 (gordy)
**	    Passwords are stored encoded.  Added timeZone, decimalChar, 
**	    dateFormat, moneyFormat and moneyPrecision.
**	23-Jul-07 (gordy)
**	    Added property dateAlias.
**	30-Jul-10 (gordy)
**	    Added property sendIntegerBooleans.
*/

private Properties
getProperties( String user, String password )
{
    boolean	tracing = trace.enabled( 3 );
    Properties	info = new Properties();

    if ( user == null )  
    {
	user = this.user;
	password = decode( this.password, this.user );
    }
    
    if ( databaseName != null )
    {
	if ( tracing )  
	    trace.write( "     " + DRV_PROP_DB + ": " + databaseName );
	info.setProperty( DRV_PROP_DB, databaseName );
    }

    if ( user != null )
    {
	if ( tracing )  
	    trace.write( "     " + DRV_PROP_USR + ": " + user );
	info.setProperty( DRV_PROP_USR, user );
    }

    if ( password != null )
    {
	if ( tracing )  
	    trace.write( "     " + DRV_PROP_PWD + ": *****" );
	info.setProperty( DRV_PROP_PWD, password );
    }

    if ( roleName != null )
    {
	if ( tracing )  
	    trace.write( "     " + DRV_PROP_ROLE + ": " + roleName );
	info.setProperty( DRV_PROP_ROLE, roleName );
    }

    if ( groupName != null )
    {
	if ( tracing )  
	    trace.write( "     " + DRV_PROP_GRP + ": " + groupName );
	info.setProperty( DRV_PROP_GRP, groupName );
    }

    if ( dbmsUser != null )
    {
	if ( tracing )  
	    trace.write( "     " + DRV_PROP_DBUSR + ": " + dbmsUser );
	info.setProperty( DRV_PROP_DBUSR, dbmsUser );
    }

    if ( dbmsPassword != null )
    {
	if ( tracing )  
	    trace.write( "     " + DRV_PROP_DBPWD + ": *****" );
	info.setProperty( DRV_PROP_DBPWD, decode( dbmsPassword, dbmsUser ) );
    }

    if ( connectionPool != null )
    {
	if ( tracing )  
	    trace.write( "     " + DRV_PROP_POOL + ": " + connectionPool );
	info.setProperty( DRV_PROP_POOL, connectionPool );
    }

    if ( autocommitMode != null )
    {
	if ( tracing )  
	    trace.write( "     " + DRV_PROP_XACM + ": " + autocommitMode );
	info.setProperty( DRV_PROP_XACM, autocommitMode );
    }

    if ( selectLoops != null )
    {
	if ( tracing )  
	    trace.write( "     " + DRV_PROP_LOOP + ": " + selectLoops );
	info.setProperty( DRV_PROP_LOOP, selectLoops );
    }

    if ( cursorMode != null )
    {
	if ( tracing )  
	    trace.write( "     " + DRV_PROP_CRSR + ": " + cursorMode );
	info.setProperty( DRV_PROP_CRSR, cursorMode );
    }

    if ( vnodeUsage != null )
    {
	if ( tracing )  
	    trace.write( "     " + DRV_PROP_VNODE + ": " + vnodeUsage );
	info.setProperty( DRV_PROP_VNODE, vnodeUsage );
    }

    if ( charEncode != null )
    {
	if ( tracing )  
	    trace.write( "     " + DRV_PROP_ENCODE + ": " + charEncode );
	info.setProperty( DRV_PROP_ENCODE, charEncode );
    }

    if ( timeZone != null )
    {
	if ( tracing )  
	    trace.write( "     " + DRV_PROP_TIMEZONE + ": " + timeZone );
	info.setProperty( DRV_PROP_TIMEZONE, timeZone );
    }

    if ( decimalChar != 0 )
    {
	if ( tracing )  
	    trace.write( "     " + DRV_PROP_DEC_CHAR + ": " + decimalChar );
	info.setProperty( DRV_PROP_DEC_CHAR, String.valueOf( decimalChar ) );
    }

    if ( dateFormat != null )
    {
	if ( tracing )  
	    trace.write( "     " + DRV_PROP_DATE_FRMT + ": " + dateFormat );
	info.setProperty( DRV_PROP_DATE_FRMT, dateFormat );
    }

    if ( moneyFormat != null )
    {
	if ( tracing )  
	    trace.write( "     " + DRV_PROP_MNY_FRMT + ": " + moneyFormat );
	info.setProperty( DRV_PROP_MNY_FRMT, moneyFormat );
    }

    if ( moneyPrecision > 0 )
    {
	String value = Integer.toString( moneyPrecision );
	
	if ( tracing )  
	    trace.write( "     " + DRV_PROP_MNY_PREC + ": " + value );
	info.setProperty( DRV_PROP_MNY_PREC, value );
    }

    if ( dateAlias != null )
    {
	if ( tracing )
	    trace.write( "     " + DRV_PROP_DATE_ALIAS + ": " + dateAlias );
	info.setProperty( DRV_PROP_DATE_ALIAS, dateAlias );
    }

    if ( sendIngresDates != null )
    {
	if ( tracing )
	    trace.write( "     " + DRV_PROP_SND_ING_DTE + 
			 ": " + sendIngresDates );
	info.setProperty( DRV_PROP_SND_ING_DTE, sendIngresDates );
    }

    if ( sendIntegerBooleans != null )
    {
	if ( tracing )
	    trace.write( "     " + DRV_PROP_SND_INT_BOOL + 
			 ": " + sendIntegerBooleans );
	info.setProperty( DRV_PROP_SND_INT_BOOL, sendIntegerBooleans );
    }

    return( info );
} // getProperties


} // class JdbcDS
