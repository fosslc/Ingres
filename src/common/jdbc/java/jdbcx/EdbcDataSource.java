/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbcx;

/*
** Name: EdbcDataSource.java
**
** Description:
**	Implements the EDBC JDBC extension class EdbcDataSource.
**
** History:
**	26-Jan-01 (gordy)
**	    Created.
**	18-Apr-01 (gordy)
**	    Made fields protected and added getHost() method for sub-
**	    classes which need their own connections independent of
**	    the standard connection described by the property values.
*/


import	java.util.Date;
import	java.util.Properties;
import	java.io.ObjectInputStream;
import	java.io.PrintWriter;
import	java.io.Serializable;
import	java.io.IOException;
import	java.sql.Connection;
import	java.sql.SQLException;
import	javax.naming.Reference;
import	javax.naming.Referenceable;
import	javax.naming.BinaryRefAddr;
import	javax.naming.StringRefAddr;
import	javax.naming.NamingException;
import	javax.sql.DataSource;
import	ca.edbc.jdbc.EdbcConnect;
import	ca.edbc.jdbc.EdbcTrace;
import	ca.edbc.jdbc.EdbcConst;


/*
** Name: EdbcDataSource
**
** Description:
**	EDBC JDBC extension class which implements the JDBC 
**	DataSource interface.
**
**	The following data source properties are supported:
**
**	dataSourceName		DataSource name.
**	description		DataSource description
**	serverName		Target machine name.
**	portNumber		Numeric port ID.
**	portName		Symbolic port ID.
**	databaseName		Database.
**	user			Username for connection
**	password		Password for connection.
**	roleName		Role used in DBMS.
**	groupName		Group used in DBMS.
**	dbmsUser		Username used in DBMS.
**	dbmsPassword		Password used in DBMS.
**	connectionPool		Server connection pooling ('off' or 'on').
**	autocommitMode		Autocommit mode ('dbms', 'single', 'multi').
**	selectLoops		Use select loops not cursors ('off' or 'on').
**
**  Interface Methods:
**
**	getConnection		Open a new connection.
**	getLoginTimeout		Returns login timeout value.
**	setLoginTimeout		Set login timeout value.
**	getLogWriter		Returns tracing output writer.
**	setLogWriter		Set tracing output writer.
**
**	getReference		Get JNDI Reference to EdbcDataSource object.
**
**  Public Methods:
**
**	getDataSourceName	Property getter/setter methods
**	setDataSourceName
**	getDescription
**	setDescription
**	getServerName
**	setServername
**	getPortNumber
**	setPortNumber
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
**
**  Protected Methods:
**
**	initReference		Init a JNDI Reference for EdbcDataSource.
**	getHost			Returns target host identifier.
**
**  Private Data:
**
**	dataSourceName		Property values
**	description
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
**
**	title			Class title for tracing.
**	tr_id			Class ID for tracing.
**	trace			DS Tracing.
**	conn_trace		Connection tracing.
**
** History:
**	26-Jan-01 (gordy)
**	    Created.
**	18-Apr-01 (gordy)
**	    Made fields protected and added getHost() method for sub-
**	    classes which need their own connections independent of
**	    the standard connection described by the property values.
*/

public class
EdbcDataSource
    implements	DataSource, Serializable, Referenceable
{

    /*
    ** Tracing
    */
    protected String		title = "EDBC-DataSource";
    protected String		tr_id = "EdbcDS";
    protected transient TraceDS	trace = null;
    protected transient TraceDS	conn_trace = null;

    /*
    ** Property values.
    */
    protected String		dataSourceName = null;
    protected String		description = null;
    protected String		serverName = null;
    protected int		portNumber = 0;
    protected String		portName = null;
    protected String		databaseName = null;
    protected String		user = null;
    protected String		password = null;
    protected String		roleName = null;
    protected String		groupName = null;
    protected String		dbmsUser = null;
    protected String		dbmsPassword = null;
    protected String		connectionPool = null;
    protected String		autocommitMode = null;
    protected String		selectLoops = null;

    /*
    ** DataSource info
    */
    protected int			timeout = 0;


/*
** Name: EdbcDataSource
**
** Description:
**	Class constructor (needed for object factory).
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
*/

public
EdbcDataSource() 
{
    initialize();
}


private void
initialize()
{
    trace = new TraceDS( "EDBCX" );
    conn_trace = new TraceDS( EdbcTrace.edbcTraceID );
    return;
}


/*
** Property getter/setter methods.
**
** Each supported property has methods to set and retrieve
** the property value.
*/

public String 
getDataSourceName() 
{ return( dataSourceName ); }

public void
setDataSourceName( String dataSourceName )
{ this.dataSourceName = dataSourceName; }

public String 
getDescription() 
{ return( description ); }

public void
setDescription( String description )
{ this.description = description; }

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
{ this.user = user; }

public String 
getPassword() 
{ return( password ); }

public void
setPassword( String password )
{ this.password = password; }

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
getDbmsUser() 
{ return( dbmsUser ); }

public void
setDbmsUser( String dbmsUser )
{ this.dbmsUser = dbmsUser; }

public String 
getDbmsPassword() 
{ return( dbmsPassword ); }

public void
setDbmsPassword( String dbmsPassword )
{ this.dbmsPassword = dbmsPassword; }

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
    return( getConnection( null, null ) );
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
**	18-Apr-01 (gordy)
**	    Extracted host:port formatting to getHost() method.
*/

public Connection
getConnection( String user, String password )
    throws SQLException
{
    Connection	conn;
    Properties	info = new Properties();
    String	host = getHost();

    if ( trace.enabled() )  trace.log(title + ".getConnection(" + user + ")");

    if ( user == null )  user = this.user;
    if ( password == null )  password = this.password;

    if ( databaseName != null )
	info.setProperty( EdbcConst.cp_db, databaseName );
    if ( user != null )
	info.setProperty( EdbcConst.cp_usr, user );
    if ( password != null )
	info.setProperty( EdbcConst.cp_pwd, password );
    if ( roleName != null )
	info.setProperty( EdbcConst.cp_role, roleName );
    if ( groupName != null )
	info.setProperty( EdbcConst.cp_grp, groupName );
    if ( dbmsUser != null )
	info.setProperty( EdbcConst.cp_dbusr, dbmsUser );
    if ( dbmsPassword != null )
	info.setProperty( EdbcConst.cp_dbpwd, dbmsPassword );
    if ( connectionPool != null )
	info.setProperty( EdbcConst.cp_pool, connectionPool );
    if ( autocommitMode != null )
	info.setProperty( EdbcConst.cp_xacm, autocommitMode );
    if ( selectLoops != null )
	info.setProperty( EdbcConst.cp_loop, selectLoops );

    if ( trace.enabled( 2 ) )
	trace.write( tr_id + ".getConnection: " + host + ", '" + 
		     databaseName + "', '" + user + "'" );

    conn = new EdbcConnect( host, info, conn_trace, timeout );
    if ( trace.enabled() )  trace.log( title + ".getConnection(): " + conn ); 
    return( conn );
} // getConnection


/*
** Name: getHost
**
** Description:
**	Returns the target host identifier composed of the
**	machine (serverName) and port (portName or portNumber)
**	formatted as needed for the EDBC Driver.
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
*/

protected String
getHost()
{
    String	host = serverName + ":"; 
	
    if ( portName != null )  
	host += portName;
    else  
	host += Integer.toString( portNumber );

    return( host );
} // getHost


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
*/

public void
setLogWriter( PrintWriter out )
    throws SQLException
{
    trace.setWriter( out );

    if ( out != null )
    {
	/*
	** Write initial trace message to log.
	*/
	Date now = new Date();
	out.println( EdbcConst.driverID + ": " + now );
    }


    if ( conn_trace.getWriter() != out )
    {
	/*
	** Since the current connection tracing may be used 
	** by a connection, a new trace object is created
	** instead of just setting the trace log.
	*/
	conn_trace = new TraceDS( EdbcTrace.edbcTraceID );
	conn_trace.setWriter( out );
    }
    return;
} // setLogWriter


/*
** Name: getReference
**
** Description:
**	Returns a JNDI Reference to this object.  Encodes the property
**	values to be retrieved by the getObjectInstance() method of the
**	class EdbcDSFactory.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Reference   A JNDI Reference.
**
** History:
**	26-Jan-01 (gordy)
**	    Created.
*/

public Reference
getReference()
    throws NamingException
{
    Reference	ref = new Reference( this.getClass().getName(),
				     "ca.edbc.jdbcx.EdbcDSFactory", null );
    initReference( ref );
    return( ref );
} // getReference


/*
** Name: initReference
**
** Description:
**	Initialize a Reference object to represent this EdbcDataSource.
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
*/

protected void
initReference( Reference ref )
{
    if ( dataSourceName != null )
	ref.add( new StringRefAddr( "dataSourceName", dataSourceName ) );

    if ( description != null )
	ref.add( new StringRefAddr( "description", description ) );

    if ( serverName != null )
	ref.add( new StringRefAddr( "serverName", serverName ) );

    if ( portName != null )
	ref.add( new StringRefAddr( "portName", portName ) );

    if ( databaseName != null )
	ref.add( new StringRefAddr( "databaseName", databaseName ) );

    if ( user != null )
	ref.add( new StringRefAddr( "user", user ) );

    if ( password != null )
	ref.add( new StringRefAddr( "password", password ) );

    if ( roleName != null )
	ref.add( new StringRefAddr( "roleName", roleName ) );

    if ( groupName != null )
	ref.add( new StringRefAddr( "groupName", groupName ) );

    if ( dbmsUser != null )
	ref.add( new StringRefAddr( "dbmsUser", dbmsUser ) );

    if ( dbmsPassword != null )
	ref.add( new StringRefAddr( "dbmsPassword", dbmsPassword ) );

    if ( connectionPool != null )
	ref.add( new StringRefAddr( "connectionPool", connectionPool ) );

    if ( autocommitMode != null )
	ref.add( new StringRefAddr( "autocommitMode", autocommitMode ) );

    if ( selectLoops != null )
	ref.add( new StringRefAddr( "selectLoops", selectLoops ) );

    if ( portNumber > 0 )
    {
	byte	ival[] = new byte[ 4 ];

	ival[0] = (byte)portNumber;
	ival[1] = (byte)(portNumber >> 8);
	ival[2] = (byte)(portNumber >> 16);
	ival[3] = (byte)(portNumber >> 24);

	ref.add( new BinaryRefAddr( "portNumber", ival ) );
    }

    return;
} // initReference


private void
readObject( ObjectInputStream in )
    throws IOException, ClassNotFoundException
{
    in.defaultReadObject();
    initialize();
    return;
}


} // class EdbcDataSource
