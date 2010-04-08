/*
** Copyright (c) 2001, 2003 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: JdbcCPDS.java
**
** Description:
**	Defines class which implements the JDBC ConnectionPoolDataSource
**	interface.
**
**  Classes
**
**	JdbcCPDS
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	18-Feb-03 (gordy)
**	    Added JDBC 3.0 connection pool properties and methods.
**	15-Jul-03 (gordy)
**	    Enhanced configuration support.  Made serializable
*/


import	java.io.ObjectInputStream;
import	java.io.Serializable;
import	java.io.IOException;
import	java.sql.SQLException;
import	javax.sql.ConnectionPoolDataSource;
import	javax.sql.PooledConnection;
import	javax.naming.Reference;
import	javax.naming.RefAddr;


/*
** Name: JdbcCPDS
**
** Description:
**	JDBC driver base class which implements the JDBC 
**	ConnectionPoolDataSource interface.
**
**	This class is a subclass of JdbcDS and so may be used as a 
**	DataSource object and supports all the properties of the 
**	JdbcDS class.  In addition, the following connection pool
**	properties are supported:
**
**	    initialPoolSize	Initial number of pooled connections.
**	    minPoolSize		Minimum number of pooled connections.
**	    maxPoolSize		Maximum number of pooled connections.
**	    maxIdleTime		Max seconds connection is pooled.
**	    propertyCycle	Connection pool check interval.
**	    maxStatements	Maximum number of pooled statements.
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
**	getPooledConnection	Generate a PooledConnection object.
**
**  Public Methods:
**
**	getInitialPoolSize	Property getter/setter methods
**	setInitialPoolSize
**	getMinPoolSize
**	setMinPoolSize
**	getMaxPoolSize
**	setMaxPoolSize
**	getMaxIdleTime
**	setMaxIdleTime
**	getPropertyCycle
**	setPropertyCycle
**	getMaxStatements
**	setMaxStatements
**
**  Protected Methods:
**
**	initReference		Init a JNDI Reference.
**	initInstance		Init JdbcDS from JNDI Reference.
**
**  Private Data:
**
**	initialPoolSize		Property values
**	minPoolSize
**	maxPoolSize
**	maxIdleTime
**	propertyCycle
**	maxStatements
**
**  Private Methods:
**
**	initialize		Initialize transient state.
**	readObject		Deserialize data source.
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Genericized and made abstract as base class for productized
**	    drivers.  Removed getReference() method which must be 
**	    implemented by sub-class.
**	18-Feb-03 (gordy)
**	    Added JDBC 3.0 connection pool properties and methods.
**	15-Jul-03 (gordy)
**	    Replaced setDriverIdentity() with load methods and removed
**	    associated fields.  Added initialize() and readObject to 
**	    support serialization.
*/

public abstract class
JdbcCPDS
    extends JdbcDS
    implements ConnectionPoolDataSource, Serializable
{

    /*
    ** Properties (serialized state).
    */
    private	int		initialPoolSize = 0;
    private	int		minPoolSize = 0;
    private	int		maxPoolSize = 0;
    private	int		maxIdleTime = 0;
    private	int		propertyCycle = 0;
    private	int		maxStatements = 0;


/*
** Name: JdbcCPDS
**
** Description:
**	Class constructor.
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
**	29-Jan-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	15-Jul-03 (gordy)
**	    Moved initialization to separate method to support serialization.
*/

protected
JdbcCPDS() 
{
    initialize();	// Initialize transient data.
} // JdbcCPDS


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
    title = trace.getTraceName() + "-ConnectionPoolDataSource[" + inst_id + "]";
    tr_id = "CPDSrc[" + inst_id + "]";
    return;
} // initialize


/*
** Property getter/setter methods.
**
** Each supported property has methods to set and retrieve
** the property value.
*/

public int 
getInitialPoolSize() 
{ return( initialPoolSize ); }

public void
setInitialPoolSize( int initialPoolSize )
{ this.initialPoolSize = initialPoolSize; }

public int 
getMinPoolSize() 
{ return( minPoolSize ); }

public void
setMinPoolSize( int minPoolSize )
{ this.minPoolSize = minPoolSize; }

public int 
getMaxPoolSize() 
{ return( maxPoolSize ); }

public void
setMaxPoolSize( int maxPoolSize )
{ this.maxPoolSize = maxPoolSize; }

public int 
getMaxIdleTime() 
{ return( maxIdleTime ); }

public void
setMaxIdleTime( int maxIdleTime )
{ this.maxIdleTime = maxIdleTime; }

public int 
getPropertyCycle() 
{ return( propertyCycle ); }

public void
setPropertyCycle( int propertyCycle )
{ this.propertyCycle = propertyCycle; }

public int 
getMaxStatements() 
{ return( maxStatements ); }

public void
setMaxStatements( int maxStatements )
{ this.maxStatements = maxStatements; }


/*
** Name: getPooledConnection
**
** Description:
**	Generate a pooled connection.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	PooledConnection    A pooled connection.
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
*/

public PooledConnection
getPooledConnection()
    throws SQLException
{
    if (trace.enabled())  trace.log( title + ".getPooledConnection()" );
    
    PooledConnection conn = 
	new JdbcCPConn( connect( (String)null, (String)null ), trace );
    
    if (trace.enabled())  trace.log(title + ".getPooledConnection(): " + conn);
    return( conn );
} // getPooledConnection


/*
** Name:getPooledConnection
**
** Description:
**	Generate a pooled connection.
**
** Input:
**	user	    User ID.
**	password    Password.
**
** Output:
**	None.
**
** Returns:
**	PooledConnection    A pooled connection.
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
*/

public PooledConnection
getPooledConnection( String user, String password )
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".getPooledConnection('" + user + "','*****')" );
    
    PooledConnection conn = new JdbcCPConn( connect( user, password ), trace );

    if (trace.enabled())  trace.log(title + ".getPooledConnection(): " + conn);
    return( conn );
} // getPooledConnection


/*
** Name: initReference
**
** Description:
**	Initialize a Reference object to represent this JdbcCPDS.
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
**	18-Feb-03 (gordy)
**	    Created.
*/

protected void
initReference( Reference ref )
{
    super.initReference( ref );

    if ( initialPoolSize != 0 )
	encodeIntRef( ref, "initialPoolSize", initialPoolSize );

    if ( minPoolSize != 0 )
	encodeIntRef( ref, "minPoolSize", minPoolSize );

    if ( maxPoolSize != 0 )
	encodeIntRef( ref, "maxPoolSize", maxPoolSize );

    if ( maxIdleTime != 0 )
	encodeIntRef( ref, "maxIdleTime", maxIdleTime );

    if ( propertyCycle != 0 )
	encodeIntRef( ref, "propertyCycle", propertyCycle );

    if ( maxStatements != 0 )
	encodeIntRef( ref, "maxStatements", maxStatements );

    return;
} // initReference


/*
** Name: initInstance
**
** Description:
**	Initialize a this JdbcCPDS object from a JNDI reference.
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
**	18-Feb-03 (gordy)
**	    Created.
*/

protected void
initInstance( Reference ref )
{
    RefAddr ra;

    super.initInstance( ref );

    if ( (ra = ref.get( "initialPoolSize" )) != null )
	initialPoolSize = decodeIntRef( ra );

    if ( (ra = ref.get( "minPoolSize" )) != null )
	minPoolSize = decodeIntRef( ra );

    if ( (ra = ref.get( "maxPoolSize" )) != null )
	maxPoolSize = decodeIntRef( ra );

    if ( (ra = ref.get( "maxIdleTime" )) != null )
	maxIdleTime = decodeIntRef( ra );

    if ( (ra = ref.get( "propertyCycle" )) != null )
	propertyCycle = decodeIntRef( ra );

    if ( (ra = ref.get( "maxStatements" )) != null )
	maxStatements = decodeIntRef( ra );

    return;
} // initInstance


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


} // class JdbcCPDS
