/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbc;

/*
** Name: EdbcConnect.java
**
** Description:
**	Implements the EDBC JDBC Connection class: EdbcConnect.
**
**  Classes
**
**	EdbcConnect	Implements the JDBC Connection interface.
**	EdbcDBInfo	Access dbmsinfo() on a connection.	
**	EdbcDBCaps	Access iidbcapabilities on a connection.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
**	 8-Sep-99 (gordy)
**	    Created new base class for class which interact with
**	    the JDBC server and extracted common data and methods.
**	    Synchronize entire request/response with JDBC server.
**	13-Sep-99 (gordy)
**	    Implemented error code support.
**	22-Sep-99 (gordy)
**	    Convert user/password to server character set when encrypting.
**	29-Sep-99 (gordy)
**	    Used DbConn lock()/unlock() for synchronization to
**	    support BLOB streams.
**	26-Oct-99 (gordy)
**	    Implemented ODBC escape parsing.
**	 2-Nov-99 (gordy)
**	    Read and save DBMS information.
**	 4-Nov-99 (gordy)
**	    Send timeout connection parameter to server.
**	12-Nov-99 (gordy)
**	    Gateways require the driver to use local time, not gmt.
**	17-Dec-99 (gordy)
**	    Use 'readonly' cursors.
**	17-Jan-00 (gordy)
**	    Connection pool parameter sent as boolean value.  Autocommit
**	    is now enable by the server at connect time and disabled at
**	    disconnect.  Replaced iidbcapabilities query with a request
**	    for a pre-defined server query.  Added readData() method to
**	    read the results of the server request.
**	19-May-00 (gordy)
**	    Check for select loops connection parameter (internal) and 
**	    set the DB connection state appropriatly.
**	13-Jun-00 (gordy)
**	    Implemented EdbcCall class and unstubbed prepareCall().
**	    REQUEST messages may return a descriptor so implemented
**	    readDesc() method.
**	30-Aug-00 (gordy)
**	    Provide a more meaningful exception when user ID or password
**	    is a zero-length string.
**	27-Sep-00 (gordy)
**	    Don't perform any action if requested readOnly state
**	    is the same as the current readOnly state.
**	 4-Oct-00 (gordy)
**	    Standardized internal tracing.
**	17-Oct-00 (gordy)
**	    Added autocommit mode connection parameter.
**	30-Oct-00 (gordy)
**	    Added support for JDBC 2.0 extensions.
**	20-Jan-01 (gordy)
**	    Added messaging protocol level 2.
**	 7-Mar-01 (gordy)
**	    Revamped the DBMS capabilities processing so accomodate
**	    support for SQL dbmsinfo() queries.
**	21-Mar-01 (gordy)
**	    Added support for distributed transactions with methods
**	    startTransaction() and prepareTransaction().
**	28-Mar-01 (gordy)
**	    Moved server connection establishment into this class and
**	    placed server and database connection code into separate
**	    methods.  Tracing now passed as parameter in constructors.  
**	    Dropped getDbConn().
**	 2-Apr-01 (gordy)
**	    Use EdbcXID, support Ingres transaction ID.
**	11-Apr-01 (gordy)
**	    Added timeout parameter to constructor.
**	16-Apr-01 (gordy)
**	    Optimize usage of Result-set Meta-data.
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
**	10-May-01 (gordy)
**	    Added support for UCS2 datatypes.
**       27-Jul-01 (loera01) Bug 105327
**          For setTransactionIsolation method, reject attempts to set the 
**          transaction level for anything other than serializable, if the 
**          SQL level of the dbms is 6.4.
**      30-Jul-01 (loera01) Bug 105356
**          Since setTransactionIsolation() and setReadOnly() may cause
**          unexpected results when invoked separately, create a new private
**          method, sendTransactionQuery, which is invoked by the two methods
**          above.  The sendTransactionQuery method takes EdbcConnect's current
**          transaction isolation and read-only settings into account before
**          sending a SET SESSION query.
**	20-Aug-01 (gordy)
**	    Added connection parameter for default cursor mode.  Support
**	    default and updatable statement concurrency modes.
**	25-Oct-01 (gordy)
**	    Only issue dbmsinfo() query for items which aren't pre-loaded
**	    into the info cache.
**	20-Feb-02 (gordy)
**	    Added dbmsInfo checks for DBMS type and protocol level.
**      30-sep-02 (loera01) Bug 108833
**          Since the intention of the setTransactionIsolaton and
**          setReadOnly methods is for the "set" query to persist in the
**          session, change "set transaction" to "set session" in the
**          sendTransactionQuery private method.
**      14-Dec-04 (rajus01) Startrak# EDJDBC94; Bug# 113625
**	    Added abort().    
*/

import	java.util.Hashtable;
import	java.util.Map;
import	java.util.Properties;
import	java.util.Vector;
import	java.sql.Connection;
import	java.sql.Statement;
import	java.sql.PreparedStatement;
import	java.sql.CallableStatement;
import	java.sql.ResultSet;
import	java.sql.DatabaseMetaData;
import	java.sql.SQLException;
import  java.sql.SQLWarning;
import	ca.edbc.util.EdbcEx;
import	ca.edbc.util.IdMap;
import	ca.edbc.io.DbConn;


/*
** Name: EdbcConnect
**
** Description:
**	EDBC JDBC Java driver class which implements the
**	JDBC Connection interface.
**
**  Interface Methods:
**
**	createStatement		Create a Statement object.
**	prepareStatement	Create a PreparedStatement object.
**	prepareCall		Create a CallableStatement object
**	nativeSQL		Translate SQL into native DBMS SQL.
**	setAutoCommit		Set autocommit state.
**	getAutoCommit		Determine autocommit state.
**	commit			Commit current transaction.
**	rollback		Rollback current transaction.
**	close			Close the connection.
**	isClosed		Determine if connection is closed.
**	getMetaData		Create a DatabaseMetaData object.
**	setReadOnly		Set readonly state
**	isReadOnly		Determine readonly state.
**	setCatalog		Set the DBMS catalog.
**	getCatalog		Retrieve the DBMS catalog.
**	setTransactionIsolation	Set the transaction isolation level.
**	getTransactionIsolation	Determine the transaction isolation level.
**	getTypeMap		Retrieve the type map.
**	setTypeMap		Set the type map.
**
**  Public Methods:
**
**	startTransaction	Start distributed transaction.
**	prepareTransaction	Prepare distributed transaction.
**	getDbmsInfo		Retrieve DBMS info and capabilities.
**
**  Protected Data
**
**	host			Host address and port.
**	database		Target database specification.
**
**  Private Data
**
**	dtmc			Distributed Transaction Management Connection?
**	autoCommit		Autocommit is ON?
**	readOnly		Connection is readonly?
**	isolationLevel		Current transaction isolationLevel.
**	type_map		Map for DBMS datatypes.
**	dbInfo			DBMS information access.
**	dbXids			Distributed XID access.
**
**  Private Methods:
**
**	disconnect		Shutdown DB connection.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
**	 8-Sep-99 (gordy)
**	    Created new base class for class which interact with
**	    the JDBC server and extracted common data and methods.
**	    Synchronize entire request/response with JDBC server.
**	 2-Nov-99 (gordy)
**	    Add getDbmsInfo(), readDbmsInfo() methods.
**	11-Nov-99 (rajus01)
**	    Add field url.
**	17-Jan-00 (gordy)
**	    JDBC server now has AUTOCOMMIT ON as its default state.
**	30-Oct-00 (gordy)
**	    Support JDBC 2.0 extensions.  Added type_map, getTypeMap(),
**	    setTypeMap(), createStatement(), prepareStatement(),
**	    prepareCall(), and getDbConn().  Removed getDbmsInfo().
**	 7-Mar-01 (gordy)
**	    Replace readDbmsInfo() method with class to perform same
**	    operation.  Add getDbmsInfo() method and companion class
**	    which implements the SQL dbmsinfo() function.  Added dbInfo
**	    field for the getDbmsInfo() method companion class.
**	21-Mar-01 (gordy)
**	    Added support for distributed transactions with methods
**	    startTransaction() and prepareTransaction().
**	28-Mar-01 (gordy)
**	    Replaced URL with host and database.  Dropped getDbConn() 
**	    as dbc now has package access.
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
**	    Added getPreparedTransactionIDs() method and constructor which
**	    establishes a DTMC.  Added dtmc flag and EdbcXids request class.
**	25-Oct-01 (gordy)
**	    Added getDbmsInfo() cover method which restricts request to cache.
*/

public class
EdbcConnect
    extends	EdbcObj
    implements	Connection
{

    /*
    ** The following require package access for meta-data.
    */
    String		host = null;
    String		database = null;
    
    private static final int	OI_SQL_LEVEL = 605;

    private boolean	dtmc = false;
    private boolean	autoCommit = true;
    private boolean	readOnly = false;
    private int		isolationLevel = TRANSACTION_SERIALIZABLE;
    private Map		type_map = new Hashtable();
    private EdbcDBInfo	dbInfo = null;
    private EdbcXids	dbXids = null;


/*
** Name EdbcConnect
**
** Description:
**	Class constructor.  Common functionality for other
**	constructors.  Initialize tracing.
**
** Input:
**	host	Target host and port.
**	trace	Connection tracing.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	18-Apr-01 (gordy)
**	    Extracted from other constructors.
*/

private
EdbcConnect( String host, EdbcTrace trace )
    throws EdbcEx
{
    super( null, trace );
    this.host = host;
    title = shortTitle + "-Connect";
    tr_id = "Conn";
} // EdbcConnect


/*
** Name: EdbcConnect
**
** Description:
**	Class constructor.  Send connection request to server
**	containing connection parameters.  Throws SQLException
**	if connection request fails.
**
** Input:
**	host	Target host and port.
**	info	Property list.
**	trace	Connection tracing.
**	timeout	Connection timeout.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	17-Sep-99 (rajus01)
**	    Added connection parameters.
**	22-Sep-99 (gordy)
**	    Convert user/password to server character set when encrypting.
**	 2-Nov-99 (gordy)
**	    Read DBMS information after establishing connection.
**	 4-Nov-99 (gordy)
**	    Send timeout connection parameter to server.
**	17-Jan-00 (gordy)
**	    Connection pool parameter sent as boolean value.  No longer
**	    need to turn on autocommit since it is the default state for
**	    the JDBC server.
**	19-May-00 (gordy)
**	    Check for select loops connection parameter (internal) and 
**	    set the DB connection state appropriatly.
**	30-Aug-00 (gordy)
**	    BUG #102472 - Fix the exception returned if user ID or
**	    password are zero-length strings.  A zero-length string
**	    was not being caught by the null test and was causing an
**	    array exception during the password encoding.  This was
**	    caught by intermediate code and turned into a 'character
**	    encoding' exception.  Now, null and zero-length valued
**	    parameters are skipped, which for user ID and password
**	    result in a reasonably specific error from the server.
**	 4-Oct-00 (gordy)
**	    Create unique ID for standardized internal tracing.
**	17-Oct-00 (gordy)
**	    Added autocommit mode connection parameter.
**	20-Jan-01 (gordy)
**	    Autocommit mode only supported at protocol level 2 or higher.
**	 7-Mar-01 (gordy)
**	    Use new class to read DBMS capabilities.
**	28-Mar-01 (gordy)
**	    Moved server connection establishment into this class and
**	    placed server and database connection code into separate
**	    methods.  Replaced dbc parameter with host.  Dropped URL
**	    as a parameter and added tracing as a parameter.
**	11-Apr-01 (gordy)
**	    Added timeout parameter for dbms connection.
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
*/

public
EdbcConnect( String host, Properties info, EdbcTrace trace, int timeout )
    throws EdbcEx
{
    this( host, trace );
    server_connect( host, false );
    dbms_connect( info, null, timeout );
} // EdbcConnect


/*
** Name: EdbcConnect
**
** Description:
**	Class constructor.  Establish a distributed transaction
**	management connection.  An XID may be provided, in which
**	case the server will attempt to assign ownership of the
**	transaction to the new connection.
**
** Input:
**	host	Target host and port.
**	info	Property list.
**	trace	Connection tracing.
**	xid	Distributed transaction ID, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	18-Apr-01 (gordy)
**	    Created.
*/

public
EdbcConnect( String host, Properties info, EdbcTrace trace, EdbcXID xid )
    throws EdbcEx
{
    this( host, trace );

    if ( xid != null )
	switch( xid.getType() )
	{
	case EdbcXID.XID_INGRES :
	case EdbcXID.XID_XA :
	    // OK!
	    break;

	default :
	    if ( trace.enabled( 1 ) ) 
		trace.write( tr_id + ": unknown XID type: " + xid.getType() );
	    throw EdbcEx.get( E_JD0012_UNSUPPORTED );
	}

    server_connect( host, true );

    if ( dbc.msg_protocol_level < JDBC_MSG_PROTO_2 )
    {
	if ( trace.enabled( 1 ) ) 
	    trace.write(tr_id + ": 2PC protocol = " + dbc.msg_protocol_level);
	disconnect();
	throw EdbcEx.get( E_JD0012_UNSUPPORTED );
    }

    dbms_connect( info, xid, 0 );
} // EdbcConnect


/*
** Name: server_connect
**
** Description:
**	Establish a connection to the JDBC Server.  If requested,
**	the connection will enable the management of distributed
**	transactions.
**
** Input:
**	host	Host address and port: "<addr>:<port>".
**	dtmc	Distributed Transaction Management Connection?
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	28-Mar-01 (gordy)
**	    Extracted from EdbcDriver.
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
*/

private void
server_connect( String host, boolean dtmc )
    throws EdbcEx
{
    byte	msg_proto = JDBC_MSG_PROTO_1;	// Assume lowest
    byte	params[] = new byte[ 5 ];
    short	param = 0;

    if ( trace.enabled( 2 ) )  
	trace.write( tr_id + ": Connecting to server: " + host ); 

    params[ param++ ] = JDBC_MSG_P_PROTO;
    params[ param++ ] = (byte)1;
    params[ param++ ] = JDBC_MSG_DFLT_PROTO;		// Request default
    
    if ( dtmc )
    {
	params[ param++ ] = JDBC_MSG_P_DTMC;
	params[ param++ ] = (byte)0;
    }

    dbc = new DbConn( host, params, param );
    title += "[" + dbc.connID() + "]";	// Add instance ID to trace ID
    tr_id += "[" + dbc.connID() + "]";

    try
    {
	for( int i = 0; i < params.length - 1; )
	{
	    byte    param_id = params[ i++ ];
	    byte    param_len = params[ i++ ];

	    switch( param_id )
	    {
	    case 0 : break;	// end of parameters

	    case JDBC_MSG_P_PROTO :
		if ( param_len != 1  ||  i >= params.length )
		{
		    if ( trace.enabled( 1 ) )  
			trace.write( "Invalid PROTO param length: " +
				     param_len + "," + params.length );
		    throw EdbcEx.get( E_JD0001_CONNECT_ERR );
		}
		
		msg_proto = params[ i++ ];
		break;

	    case JDBC_MSG_P_DTMC :
		if ( param_len != 0 )
		{
		    if ( trace.enabled( 1 ) )  
			trace.write( "Invalid DTMC param length: " +
				     param_len + "," + params.length );
		    throw EdbcEx.get( E_JD0001_CONNECT_ERR );
		}
		this.dtmc = true;
		break;

	    default :
		if ( trace.enabled( 1 ) )
		    trace.write( "Invalid param ID: " + param_id );
		throw EdbcEx.get( E_JD0001_CONNECT_ERR );
	    }
	}

	if ( msg_proto < JDBC_MSG_PROTO_1  ||  msg_proto > JDBC_MSG_DFLT_PROTO )
	{
	    if ( trace.enabled( 1 ) )
		trace.write( tr_id + ": Invalid MSG protocol: " + msg_proto );
	    throw EdbcEx.get( E_JD0001_CONNECT_ERR );
	}

	if ( dtmc  &&  ! this.dtmc )
	{
	    if ( trace.enabled( 1 ) )
		trace.write( tr_id + ": Could not establish DTM connection!" );
	    throw EdbcEx.get( E_JD0012_UNSUPPORTED );
	}

	if ( this.dtmc  &&  ! dtmc )	// should not happen!
	{
	    if ( trace.enabled( 1 ) )
		trace.write( tr_id + ": DTM connection not requested!" );
	    throw EdbcEx.get( E_JD0001_CONNECT_ERR );
	}
    }
    catch( EdbcEx ex )
    {
	dbc.close();
	throw ex;
    }

    if ( trace.enabled( 2 ) )  
    {
	trace.write( tr_id + ": connected to server" );

	if ( trace.enabled( 3 ) )
	{
	    trace.write( tr_id + ": Connection parameters negotiated:" );
	    trace.write( "    MSG protocol: " + msg_proto );
	    if ( this.dtmc )  trace.write( "    DTM Connection" );
	}
    }

    dbc.msg_protocol_level = msg_proto;
    return;
} // server_connect


/*
** Name: dbms_connect
**
** Description:
**	Establish connection with DBMS.
**
** Input:
**	info	Connection property list.
**	xid	Distributed transaction ID.
**	timeout	Connection timeout.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	28-Mar-01 (gordy)
**	    Separated from constructor.
**	11-Apr-01 (gordy)
**	    Added timeout parameter to replace DriverManager reference.
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
**	10-May-01 (gordy)
**	    Check DBMS capabilities relevant to driver configuration.
**	20-Aug-01 (gordy)
**	    Added connection parameter for default cursor mode.
**	20-Feb-02 (gordy)
**	    Added dbmsInfo checks for DBMS type and protocol level.
*/

private void
dbms_connect( Properties info, EdbcXID xid, int timeout )
    throws EdbcEx
{
    if ( trace.enabled( 2 ) )  trace.write( tr_id + ": Connecting to database" ); 

    try
    {
	/*
	** Send connection request and parameters.
	*/
	dbc.begin( JDBC_MSG_CONNECT );
	if ( propInfo.length > 0  &&  trace.enabled( 3 ) )  
	    trace.write( tr_id + ": sending connection parameters" );

	for( int i = 0; i < propInfo.length; i++ )
	{
	    String value = info.getProperty( propInfo[ i ].name );
	    if ( value == null  ||  value.length() == 0 )  continue;
	    
	    switch( propInfo[ i ].cpID )
	    {
	    case JDBC_CP_DATABASE :
		database = value;
		dbc.write( propInfo[ i ].cpID );
		dbc.write( value );
		break;

	    case JDBC_CP_PASSWORD :
		byte	pwd[];
		String	user = info.getProperty( "user" );
		if ( user == null  ||  user.length() == 0 )  
		    break;	// no user, no password

		pwd = dbc.encode( user, value );
		dbc.write( propInfo[ i ].cpID );
		dbc.write( pwd );
		value = "*****";
		break;

 	    case JDBC_CP_DBPASSWORD :
 		dbc.write( propInfo[ i ].cpID );
 		dbc.write( value );
		value = "*****";
 		break;

	    case JDBC_CP_CONNECT_POOL :
		if ( value.equalsIgnoreCase( "off" ) )
		{
		    dbc.write( propInfo[ i ].cpID );
		    dbc.write( (short)1 );
		    dbc.write( JDBC_CPV_POOL_OFF );
		}
		else  if ( value.equalsIgnoreCase( "on" ) )
		{
		    dbc.write( propInfo[ i ].cpID );
		    dbc.write( (short)1 );
		    dbc.write( JDBC_CPV_POOL_ON );
		}
		break;

	    case JDBC_CP_AUTO_MODE :
		if ( dbc.msg_protocol_level < JDBC_MSG_PROTO_2 )
		{
		    if ( value.equalsIgnoreCase( "single" )  ||
			 value.equalsIgnoreCase( "multi" ) )
		    {
			if ( trace.enabled( 1 ) ) 
			    trace.write( tr_id + ": autocommit protocol = " + 
					 dbc.msg_protocol_level );
			throw EdbcEx.get( E_JD0012_UNSUPPORTED );
		    }
		}
		else  if ( value.equalsIgnoreCase( "dbms" ) )
		{
		    dbc.write( propInfo[ i ].cpID );
		    dbc.write( (short)1 );
		    dbc.write( JDBC_CPV_XACM_DBMS );
		}
		else  if ( value.equalsIgnoreCase( "single" ) )
		{
		    dbc.write( propInfo[ i ].cpID );
		    dbc.write( (short)1 );
		    dbc.write( JDBC_CPV_XACM_SINGLE );
		}
		else  if ( value.equalsIgnoreCase( "multi" ) )
		{
		    dbc.write( propInfo[ i ].cpID );
		    dbc.write( (short)1 );
		    dbc.write( JDBC_CPV_XACM_MULTI );
		}
		break;

	    case JDBC_CP_SELECT_LOOP :
		/*
		** This is a local connection property
		** and is not forwarded to the server.
		*/
		if ( value.equalsIgnoreCase( "off" ) )
		    dbc.select_loops = false;
		else  if ( value.equalsIgnoreCase( "on" ) )
		    dbc.select_loops = true;
		break;

	    case JDBC_CP_CURSOR_MODE :
		/*
		** This is a local connection property
		** and is not forwarded to the server.
		*/
		if ( value.equalsIgnoreCase( "dbms" ) )
		    dbc.cursor_mode = DbConn.CRSR_DBMS;
		else  if ( value.equalsIgnoreCase( "update" ) )
		    dbc.cursor_mode = DbConn.CRSR_UPDATE;
		else  if ( value.equalsIgnoreCase( "readonly" ) )
		    dbc.cursor_mode = DbConn.CRSR_READONLY;
		break;

	    default :
		dbc.write( propInfo[ i ].cpID );
		dbc.write( value );
		break;
	    }

	    if ( trace.enabled( 3 ) )  
		trace.write( tr_id + ":     " + 
			     IdMap.get( propInfo[ i ].cpID, cpMap ) + 
			     " '" + value + "'" );
	}

	/*
	** JDBC timeout of 0 means no timeout (full blocking).
	** Only send timeout if non-blocking.
	*/
	if ( timeout != 0 )
	{
	    /*
	    ** Ingres JDBC driver interprets a negative timeout
	    ** as milli-seconds.  Otherwise, convert seconds to
	    ** milli-seconds.
	    */
	    timeout = (timeout < 0) ? -timeout : timeout * 1000;
	    dbc.write( JDBC_CP_TIMEOUT );
	    dbc.write( (short)4 );
	    dbc.write( timeout );

	    if ( trace.enabled( 3 ) )  
		trace.write( tr_id + ":     " + 
			     IdMap.get( JDBC_CP_TIMEOUT, cpMap ) + 
			     " " + timeout );
	}

	if ( xid != null )
	{
	    if ( trace.enabled( 3 ) )  
	    {
		trace.write( tr_id + ":     " +
			     IdMap.get( JDBC_CP_II_XID, cpMap ) );
		xid.trace( trace );
	    }

	    /*
	    ** Autocommit is disabled when connecting to
	    ** an existing distributed transaction.
	    */
	    autoCommit = false;

	    switch( xid.getType() )
	    {
	    case EdbcXID.XID_INGRES :
		dbc.write( JDBC_CP_II_XID );
		dbc.write( (short)8 );
		dbc.write( (int)xid.getXId() );
		dbc.write( (int)(xid.getXId() >> 32) );
		break;

	    case EdbcXID.XID_XA :
		dbc.write( JDBC_CP_XA_FRMT );
		dbc.write( (short)4 );
		dbc.write( xid.getFormatId() );
		dbc.write( JDBC_CP_XA_GTID );
		dbc.write( xid.getGlobalTransactionId() );
		dbc.write( JDBC_CP_XA_BQUAL );
		dbc.write( xid.getBranchQualifier() );
		break;
	    }
	}

	dbc.done( true );
	readResults();

	/*
	** Load the DBMS capability information, but not for
	** Distributed Transaction Management Connections
	** (which do not start with autocommit enabled).
	*/
	if ( dtmc )
	    autoCommit = false;
	else
	    (new EdbcDBCaps( dbc, trace )).readDBCaps();
    }
    catch( EdbcEx ex ) 
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( tr_id + ": error connecting to database" );
	disconnect();
	throw ex;
    }

    /*
    ** Check DBMS capabilities needed for driver configuration.
    */
    if ( ! dtmc )
    {
	String value;

	if ( (value = dbc.getDbmsInfo( "DBMS_TYPE" )) != null )
	    dbc.is_ingres = value.equalsIgnoreCase( "INGRES" );

	if ( (value = dbc.getDbmsInfo( "JDBC_CONNECT_LEVEL" )) != null )
	    try { dbc.db_protocol_level = Byte.parseByte( value ); }
	    catch( Exception ignore ) {}

	if ( (value = dbc.getDbmsInfo( "OPEN_SQL_DATES" )) != null )  
	    dbc.use_gmt_tz = false;	// TODO: check for 'LEVEL 1'

	if ( (value = dbc.getDbmsInfo( "NATIONAL_CHARACTER_SET" )) != null  &&
	     value.length() >= 1 )
	{
	    char yes_no = value.charAt(0);
	    dbc.ucs2_supported = (yes_no == 'Y' || yes_no == 'y');
	}
    }

    if ( trace.enabled( 2 ) )  trace.write( tr_id + ": connected to database" );
    return;
} // EdbcConnect


/*
** Name: finalize
**
** Description:
**	Make sure the database connection is releases.
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
**	 7-Jun-99 (gordy)
**	    Created.
*/

protected void
finalize()
    throws Throwable
{
    disconnect();
    super.finalize();
    return;
} // finalize


/*
** Name: disconnect
**
** Description:
**	Shutdown the database connection.
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
**	 5-May-99 (gordy)
**	    Created.
**	25-Jan-00 (rajus01)
**	    AutoCommit is set to false after closing the connection.
*/

private void
disconnect()
{
    if ( ! dbc.isClosed() )
    {
	dbc.close();
	if ( trace.enabled( 2 ) )  trace.write( tr_id + ": disconnected" );
    }

    autoCommit = false;
    return;
} // disconnect

/*
** Name: abort
**
** Description:
**	Abort the database connection.
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
**	 14-Dec-04 (rajus01) Startrak# EDJDBC94, Bug# 113625
**	    Created.
*/

public void 
abort()
{
    if ( trace.enabled( 2 ) )  
	trace.write( tr_id + ": Closing the socket"); 
    if( dbc != null )
	dbc.abort();
    return;
}
/*
** Name: isClosed
**
** Description:
**	Determine if the connection is closed.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if connection closed, false otherwise.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
*/

public boolean
isClosed()
    throws SQLException
{
    if ( trace.enabled() )  trace.log(title + ".isClosed(): " + dbc.isClosed());
    return( dbc.isClosed() );
} // isClosed

/*
** Name: close
**
** Description:
**	Close the connection.
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
**	 5-May-99 (gordy)
**	    Created.
**	 8-Sep-99 (gordy)
**	    Synchronize on DbConn.
**	29-Sep-99 (gordy)
**	    Used DbConn lock()/unlock() for synchronization.
**	17-Jan-00 (gordy)
**	    JDBC Server will now disable autocommit during disconnect.
**	13-Oct-00 (gordy)
**	    Return errors from close.
*/

public void
close()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".close()" );
    warnings = null;
    if ( dbc.isClosed() )  return;	// Nothing to do

    dbc.lock();
    try
    {
	byte msg_id;

	/*
	** Send disconnect request and read results.
	*/
	dbc.begin( JDBC_MSG_DISCONN );
	dbc.done( true );
	readResults();
    }
    catch( EdbcEx ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".close(): error closing connection" );
	if ( trace.enabled( 1 ) )  ex.trace( trace );
	throw ex;
    }
    finally 
    { 
	dbc.unlock(); 
	disconnect();
    }

    return;
} // close


/*
** Name: createStatement
**
** Description:
**	Creates a JDBC Statement object associated with the
**	connection.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Statement   A new Statement object.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
**	30-Oct-00 (gordy)
**	    Need to pass Connection object to statement.  The
**	    associated DbConn can be obtained via getDbConn().
**	    Pass default ResultSet type and concurrency.
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
**	20-Aug-01 (gordy)
**	    Use default concurrency.
*/

public Statement
createStatement()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".createStatement()" );
    warnings = null;

    if ( dtmc )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": not permitted when DTMC" );
	throw EdbcEx.get( E_JD0012_UNSUPPORTED );
    }

    EdbcStmt stmt = new EdbcStmt( this, ResultSet.TYPE_FORWARD_ONLY, 
					EdbcSQL.CONCUR_DEFAULT );
    if ( trace.enabled() )  
	trace.log( title + ".createStatement(): " + stmt );
    return( stmt );
} // createStatement


/*
** Name: createStatement
**
** Description:
**	Creates a JDBC Statement object associated with the
**	connection.
**
** Input:
**	type	    ResultSet type.
**	concurrency ResultSet concurrency.
**
** Output:
**	None.
**
** Returns:
**	Statement   A new Statement object.
**
** History:
**	30-Oct-00 (gordy)
**	    Created.
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
**	20-Aug-01 (gordy)
**	    Support updatable concurrency.
*/

public Statement
createStatement( int type, int concurrency )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".createStatement(" + type + "," + concurrency + ")" );
    warnings = null;

    if ( dtmc )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": not permitted when DTMC" );
	throw EdbcEx.get( E_JD0012_UNSUPPORTED );
    }

    if ( type != ResultSet.TYPE_FORWARD_ONLY  ||  
	 ( concurrency != ResultSet.CONCUR_READ_ONLY  &&
	   concurrency != ResultSet.CONCUR_UPDATABLE ) )
	warnings = EdbcEx.getWarning( E_JD0017_RS_CHANGED );

    EdbcStmt stmt = new EdbcStmt( this, ResultSet.TYPE_FORWARD_ONLY, 
	(concurrency == ResultSet.CONCUR_UPDATABLE) ? EdbcSQL.CONCUR_UPDATE
						    : EdbcSQL.CONCUR_READONLY );
    
    if ( trace.enabled() )  trace.log(title + ".createStatement(): " + stmt);
    return( stmt );
} // createStatement


/*
** Name: prepareStatement
**
** Description:
**	Creates a JDBC PreparedStatement object associated with the
**	connection.
**
** Input:
**	sql	Query text.
**
** Output:
**	None.
**
** Returns:
**	PreparedStatement   A new PreparedStatement object.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
**	30-Oct-00 (gordy)
**	    Need to pass Connection object to statement.  The
**	    associated DbConn can be obtained via getDbConn().
**	    Pass default ResultSet type and concurrency.
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
**	20-Aug-01 (gordy)
**	    Use default concurrency.
*/

public PreparedStatement
prepareStatement( String sql )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".prepareStatement( " + sql + " )" );
    warnings = null;

    if ( dtmc )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": not permitted when DTMC" );
	throw EdbcEx.get( E_JD0012_UNSUPPORTED );
    }

    EdbcPrep prep = new EdbcPrep( this, sql, ResultSet.TYPE_FORWARD_ONLY, 
					     EdbcSQL.CONCUR_DEFAULT );
    if ( trace.enabled() )  
	trace.log( title + ".prepareStatement(): " + prep );
    return( prep );
} // prepareStatement


/*
** Name: prepareStatement
**
** Description:
**	Creates a JDBC PreparedStatement object associated with the
**	connection.
**
** Input:
**	sql	Query text.
**	type	    ResultSet type.
**	concurrency ResultSet concurrency.
**
** Output:
**	None.
**
** Returns:
**	PreparedStatement   A new PreparedStatement object.
**
** History:
**	30-Oct-00 (gordy)
**	    Created.
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
**	20-Aug-01 (gordy)
**	    Support updatable concurrency.
*/

public PreparedStatement
prepareStatement( String sql, int type, int concurrency )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".prepareStatement('" + sql + "'," +
			   type + "," + concurrency + ")" );
    warnings = null;

    if ( dtmc )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": not permitted when DTMC" );
	throw EdbcEx.get( E_JD0012_UNSUPPORTED );
    }

    if ( 
	 type != ResultSet.TYPE_FORWARD_ONLY  ||  
	 (concurrency != ResultSet.CONCUR_READ_ONLY  &&
	  concurrency != ResultSet.CONCUR_UPDATABLE) 
       )
	warnings = EdbcEx.getWarning( E_JD0017_RS_CHANGED );

    EdbcPrep prep = new EdbcPrep( this, sql, ResultSet.TYPE_FORWARD_ONLY, 
	(concurrency == ResultSet.CONCUR_UPDATABLE) ? EdbcSQL.CONCUR_UPDATE
						    : EdbcSQL.CONCUR_READONLY );
    
    if ( trace.enabled() )  trace.log(title + ".prepareStatement(): " + prep);
    return( prep );
} // prepareStatement


/*
** Name: prepareCall
**
** Description:
**	Creates a JDBC CallableStatement object associated with the
**	connection.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	CallableStatement   A new CallableStatement object.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
**	13-Jun-00 (gordy)
**	    Unstubbed.
**	30-Oct-00 (gordy)
**	    Need to pass Connection object to statement.  The
**	    associated DbConn can be obtained via getDbConn().
**	20-Jan-01 (gordy)
**	    Callable statements not supported until protocol level 2.
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
*/

public CallableStatement 
prepareCall( String sql )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".prepareCall( " + sql + " )" );
    warnings = null;

    if ( dtmc )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": not permitted when DTMC" );
	throw EdbcEx.get( E_JD0012_UNSUPPORTED );
    }

    if ( dbc.msg_protocol_level < JDBC_MSG_PROTO_2 )
    {
	if ( trace.enabled( 1 ) ) 
	    trace.write( tr_id + ": protocol = " + dbc.msg_protocol_level );
	throw EdbcEx.get( E_JD0012_UNSUPPORTED );
    }

    EdbcCall call = new EdbcCall( this, sql );
    
    if ( trace.enabled() )  trace.log( title + ".prepareCall()" + call );
    return( call );
} // prepareCall


/*
** Name: prepareCall
**
** Description:
**	Creates a JDBC CallableStatement object associated with the
**	connection.
**
** Input:
**	type	    ResultSet type.
**	concurrency ResultSet concurrency.
**
** Output:
**	None.
**
** Returns:
**	CallableStatement   A new CallableStatement object.
**
** History:
**	30-Oct-00 (gordy)
**	    Created.
**	20-Jan-01 (gordy)
**	    Callable statements not supported until protocol level 2.
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
*/

public CallableStatement 
prepareCall( String sql, int type, int concurrency )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".prepareCall('" + sql + "'," +
			   type + "," + concurrency + ")" );
    warnings = null;

    if ( dtmc )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": not permitted when DTMC" );
	throw EdbcEx.get( E_JD0012_UNSUPPORTED );
    }

    if ( dbc.msg_protocol_level < JDBC_MSG_PROTO_2 )
    {
	if ( trace.enabled( 1 ) ) 
	    trace.write( tr_id + ": protocol = " + dbc.msg_protocol_level );
	throw EdbcEx.get( E_JD0012_UNSUPPORTED );
    }

    if ( type != ResultSet.TYPE_FORWARD_ONLY  ||
	 concurrency != ResultSet.CONCUR_READ_ONLY )
	warnings = EdbcEx.getWarning( E_JD0017_RS_CHANGED );

    EdbcCall call = new EdbcCall( this, sql );
    if ( trace.enabled() )  trace.log( title + ".prepareCall()" + call );
    return( call );
} // prepareCall


/*
** Name: getAutoCommit
**
** Description:
**	Determine the autocommit state.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if autocommit is ON, false if OFF.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
*/

public boolean
getAutoCommit()
    throws SQLException
{
    if ( trace.enabled() )  trace.log(title + ".getAutoCommit(): " + autoCommit);
    return( autoCommit );
} // getAutoCommit

/*
** Name: setAutoCommit
**
** Description:
**	Set the autocommit state.
**
** Input:
**	autocommit  True to set autocommit ON, false for OFF.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 5-May-99 (gordy)
**	    Created.
**	 8-Sep-99 (gordy)
**	    Synchronize on DbConn.
**	29-Sep-99 (gordy)
**	    Used DbConn lock()/unlock() for synchronization.
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
*/

public void
setAutoCommit( boolean autoCommit )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setAutoCommit( " + autoCommit + " )" );
    warnings = null;
    if ( autoCommit == this.autoCommit)  return;    // Nothing to do

    if ( dtmc )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": not permitted when DTMC" );
	throw EdbcEx.get( E_JD0012_UNSUPPORTED );
    }

    dbc.lock();
    try
    {
	dbc.begin( JDBC_MSG_XACT );
	dbc.write( (short)(autoCommit ? JDBC_XACT_AC_ENABLE 
				      : JDBC_XACT_AC_DISABLE) );
	dbc.done( true );
	readResults();
	this.autoCommit = autoCommit;
    }
    catch( EdbcEx ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".setAutoCommit(): error changing autocommit" );
	if ( trace.enabled( 1 ) )  ex.trace( trace );
	throw ex;
    }
    finally 
    { 
	dbc.unlock(); 
    }

    return;
} // setAutoCommit

/*
** Name: commit
**
** Description:
**	Commit the current transaction
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
**	 5-May-99 (gordy)
**	    Created.
**	 8-Sep-99 (gordy)
**	    Synchronize on DbConn.
**	29-Sep-99 (gordy)
**	    Used DbConn lock()/unlock() for synchronization.
*/

public void
commit()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".commit()" );
    warnings = null;
    if ( autoCommit )  return;  // Nothing to do

    dbc.lock();
    try
    {
	dbc.begin( JDBC_MSG_XACT );
	dbc.write( JDBC_XACT_COMMIT );
	dbc.done( true );
	readResults();
    }
    catch( EdbcEx ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".commit(): error committing transaction" );
	if ( trace.enabled( 1 ) )  ex.trace( trace );
	throw ex;
    }
    finally 
    { 
	dbc.unlock(); 
    }

    return;
} // commit

/*
** Name: rollback
**
** Description:
**	Rollback the current transaction
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
**	 5-May-99 (gordy)
**	    Created.
**	 8-Sep-99 (gordy)
**	    Synchronize on DbConn.
**	29-Sep-99 (gordy)
**	    Used DbConn lock()/unlock() for synchronization.
*/

public void
rollback()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".rollback()" );
    warnings = null;
    if ( autoCommit )  return;  // Nothing to do

    dbc.lock();
    try
    {
	dbc.begin( JDBC_MSG_XACT );
	dbc.write( JDBC_XACT_ROLLBACK );
	dbc.done( true );
	readResults();
    }
    catch( EdbcEx ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".rollback(): error rolling back transaction" );
	if ( trace.enabled( 1 ) )  ((EdbcEx)ex).trace( trace );
	throw ex;
    }
    finally 
    { 
	dbc.unlock(); 
    }

    return;
} // rollback

/*
** Name: nativeSQL
**
** Description:
**	Translates the input SQL query and returns the query in
**	native DBMS grammer.
**
** Input:
**	sql	SQL query.
**
** Output:
**	None.
**
** Returns:
**	String	Native DBMS query.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
**	12-Nov-99 (gordy)
**	    Use configured timezone.
**	31-Oct-01 (gordy)
**	    Timezone now passed to EdbcSQL.
*/

public String
nativeSQL( String sql )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".nativeSQL( " + sql + " )" );
    EdbcSQL parser = new EdbcSQL( sql, getConnDateFormat().getTimeZone() );
    sql = parser.parseSQL( true );
    if ( trace.enabled() )  trace.log( title + ".nativeSQL: " + sql );
    return( sql );
} // nativeSQL

/*
** Name: getMetaData
**
** Description:
**	Creates a JDBC DatabaseMetaData object associated with the
**	connection.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	DatabaseMetaData    A new DatabaseMetaData object.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
**	11-Nov-99 (rajus01)
**	    EdbcMetaData takes 'EdbcConnect' and 'url' as parameters.
**	28-Mar-01 (gordy)
**	    URL no longer available.
*/

public DatabaseMetaData
getMetaData()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getMetaData()" );
    return( new EdbcMetaData( this ) );
} // getMetaData

/*
** Name: isReadOnly
**
** Description:
**	Determine the readonly state of a connection.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if connection is readonly, false otherwise.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
*/

public boolean
isReadOnly()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".isReadOnly(): " + readOnly );
    return( readOnly );
} // isReadOnly

/*
** Name: setReadOnly
**
** Description:
**	Set the readonly state of a connection.
**
** Input:
**	readOnly    True for readonly, false otherwise.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
**	27-Sep-00 (gordy)
**	    Don't perform any action if requested state
**	    is the same as the current state.
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
**       29-Jul-01 (loera01) Bug 105356
**          Rather than just sending a SET SESSION [READ|WRITE] [aaa] query, 
**          send the fully qualified command SET SESSION READ [ONLY|WRITE], 
**          TRANSACTION ISOLATION [aaa] via the new sendTransactionQuery 
**          method.  Only do so if the read-only setting is different than 
**          current.
*/

public synchronized void
setReadOnly( boolean readOnly )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setReadOnly( " + readOnly + " )" );
    warnings = null;
    if ( this.readOnly == readOnly )  return;
    
    if ( dtmc )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": not permitted when DTMC" );
	throw EdbcEx.get( E_JD0012_UNSUPPORTED );
    }

    sendTransactionQuery( readOnly, this.isolationLevel );
    return;
} // setReadOnly

/*
** Name: getTransactionIsolation
**
** Description:
**	Determine the transaction isolation level.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Transaction isolation level.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
*/

public int
getTransactionIsolation()
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getTransactionIsolation: " + isolationLevel );
    return( isolationLevel );
} // getTransactionIsolation

/*
** Name: setTransactionIsolation
**
** Description:
**	Set the transaction isolation level.  The JDBC Connection
**	interface defines the following transaction isolation
**	levels:
**
**	    TRANSACTION_NONE
**	    TRANSACTION_READ_UNCOMMITTED
**	    TRANSACTION_READ_COMMITTED
**	    TRANSACTION_REPEATABLE_READ
**	    TRANSACTION_SERIALIZABLE
**
** Input:
**	level	Transaction isolation level.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
**       27-Jul-01 (loera01) Bug 105327
**          Reject attempts to set the transaction level for anything other
**          than serializable, if the SQL level of the dbms is 6.4.
**       29-Jul-01 (loera01) Bug 105356
**          Rather than just sending a SET SESSION TRANSACTION ISOLATION
**          [aaa] query, send the fully qualified command SET SESSION READ 
**          [ONLY|WRITE], TRANSACTION ISOLATION [aaa] via the new 
**          sendTransactionQuery method.  Only do so if the read-only setting 
**          is different than current.
**	14-Jul-04 (rajus01) Startrak Issue #13202055 problem #EDJDBC85 
**	     When accessing EDBC server from JAVA IDEs such as Unify and WSAD
**	    'Invalid parameter value" SQLexception is thrown. Since the only
**	     isolation level supported by the EDBC server is SERIALIZABLE, 
**	     the transaction level is not changed, because the default 
**	     connection transaction level is SERIALIZABLE anyway. The
**	     SQLexception is not thrown, instead a SQLWarning message is
**	     generated. 
**
**	     SPECIAL NOTE: JDBC spec permits more restrictive transaction 
**	     isolation levels when the input transaction level is not supported
**	     by the target DBMS server.
*/

public synchronized void
setTransactionIsolation( int level )
    throws SQLException
{
    String      value;
    int         sqlLevel = OI_SQL_LEVEL;

    if ( trace.enabled() )  
	trace.log( title + ".setTransactionIsolation( " + level + " )" );
    warnings = null;
    if ( level == this.isolationLevel )  return;

    if ( dtmc )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": not permitted when DTMC" );
	throw EdbcEx.get( E_JD0012_UNSUPPORTED );
    }

    /* 
    ** If the SQL level is 6.4, only allow TRANSACTION_SERIALIZABLE.
    */
    if ( ( value = dbc.getDbmsInfo( "INGRES/SQL_LEVEL" ) ) != null )  
        sqlLevel = Integer.parseInt( value );        
    if ( level != TRANSACTION_SERIALIZABLE  &&  sqlLevel < OI_SQL_LEVEL )
    {
	if ( trace.enabled(3) )
	{
	    trace.write(tr_id + ": Requested transaction isolation level is not supported" );
	}
	setWarning( new SQLWarning( 
			EdbcEx.get(E_JD0012_UNSUPPORTED).getMessage())); 
	return;

    }
    
    sendTransactionQuery( this.readOnly, level );
    return;
} // setTransactionIsolation


/*
** Name: sendTransactionQuery
**
** Description:
**	Build and send "set session" query that includes both isolation 
**      level and "read only" or "read/write" components.  Query will be in 
**      this form:
**          SET SESSION [READ ONLY|WRITE] [,ISOLATION LEVEL isolation_level] 
**
** Input:
**      readOnly        True if read-only, false if read/write
**      isolationLevel  One of four values:
**                      TRANSACTION_SERIALIZABLE 
**                      TRANSACTION_READ_UNCOMMITTED 
**                      TRANSACTION_READ_COMMITTED 
**                      TRANSACTION_REPEATABLE_READ
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	27-Jul-01 (loera01) 
**	    Created.
**      30-sep-02 (loera01) Bug 108833
**          Since the intention of the setTransactionIsolaton and
**          setReadOnly methods is for the "set" query to persist in the
**          session, change "set transaction" to "set session".
*/
private synchronized void
sendTransactionQuery( boolean readOnly, int isolationLevel )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".sendTransactionQuery( " + 
		   readOnly + " , " + isolationLevel + " )" );

    String sql = "set session read ";
    sql += readOnly ? "only" : " write";
    sql += ", isolation level ";

    try
    {
	Statement stmt = createStatement();
	
	switch( isolationLevel )
	{
	case TRANSACTION_READ_UNCOMMITTED : sql += "read uncommitted"; break;
	case TRANSACTION_READ_COMMITTED :   sql += "read committed"; break;
	case TRANSACTION_REPEATABLE_READ :  sql += "repeatable read"; break;
	case TRANSACTION_SERIALIZABLE :	    sql += "serializable"; break;
	
	default :
	    throw EdbcEx.get( E_JD000A_PARAM_VALUE );
	}

	stmt.executeUpdate( sql );
	warnings = stmt.getWarnings();
	stmt.close();
	this.readOnly = readOnly;
        this.isolationLevel = isolationLevel;
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".sendTransactionQuery(): failed!" );
	if ( trace.enabled( 1 ) )  ((EdbcEx)ex).trace( trace );
	throw ex;
    }
    return;
} // sendTransactionQuery


/*
** Name: getCatalog
**
** Description:
**	Retrieve the DBMS catalog.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	    The DBMS catalog.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
*/

public String
getCatalog()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getCatalog(): " + null );
    return( null );
} // getCatalog

/*
** Name: setCatalog
**
** Description:
**	Set the DBMS catalog.
**
** Input:
**	catalog	    DBMS catalog.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
*/

public void
setCatalog( String catalog )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setCatalog( " + catalog + " ): ignored" );
    return;
} // setCatalog


/*
** Name: getTypeMap
**
** Description:
**	Retrieve the type map.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Map	Type map.
**
** History:
**	30-Oct-00 (gordy)
**	    Created.
*/

public Map
getTypeMap()
{
    if ( trace.enabled() )  trace.log( title + ".getTypeMap()" );
    return( type_map );
} // getTypeMap


/*
** Name; setTypeMap
**
** Description:
**	Set the type map.
**
** Input:
**	map	Type map.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	30-Oct-00 (gordy)
**	    Created.
*/

public void
setTypeMap( Map map )
{
    if ( trace.enabled() )  trace.log( title + ".setTypeMap()" );
    type_map = map;
    return;
} // setTypeMap


/*
** Name: startTransaction
**
** Description:
**	Start a distributed transaction.  No transaction, autocommit
**	or normal, may be active.
**
** Input:
**	xid	Edbc XID.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	21-Mar-01 (gordy)
**	    Created.
**	28-Mar-01 (gordy)
**	    Check protocol level for server support.
**	 2-Apr-01 (gordy)
**	    Use EdbcXID, support Ingres transaction ID.
**	11-Apr-01 (gordy)
**	    Trace transaction ID.
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
*/

public void
startTransaction( EdbcXID xid )
    throws SQLException
{
    warnings = null;

    if ( trace.enabled() )  
    {
	trace.log( title + ".startTransaction()" );
	if ( trace.enabled( 3 ) )  xid.trace( trace );
    }

    if ( dtmc )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": not permitted when DTMC" );
	throw EdbcEx.get( E_JD0012_UNSUPPORTED );
    }

    if ( dbc.msg_protocol_level < JDBC_MSG_PROTO_2 )
    {
	if ( trace.enabled( 1 ) ) 
	    trace.write( tr_id + ": protocol = " + dbc.msg_protocol_level );
	throw EdbcEx.get( E_JD0012_UNSUPPORTED );
    }

    switch( xid.getType() )
    {
    case EdbcXID.XID_INGRES :
    case EdbcXID.XID_XA :
	// OK!
	break;

    default :
	if ( trace.enabled( 1 ) ) 
	    trace.write( tr_id + ": unknown XID type: " + xid.getType() );
	throw EdbcEx.get( E_JD0012_UNSUPPORTED );
    }

    dbc.lock();
    try
    {
	dbc.begin( JDBC_MSG_XACT );
	dbc.write( JDBC_XACT_BEGIN );

	switch( xid.getType() )
	{
	case EdbcXID.XID_INGRES :
	    dbc.write( JDBC_XP_II_XID );
	    dbc.write( (short)8 );
	    dbc.write( (int)xid.getXId() );
	    dbc.write( (int)(xid.getXId() >> 32) );
	    break;

	case EdbcXID.XID_XA :
	    dbc.write( JDBC_XP_XA_FRMT );
	    dbc.write( (short)4 );
	    dbc.write( xid.getFormatId() );
	    dbc.write( JDBC_XP_XA_GTID );
	    dbc.write( xid.getGlobalTransactionId() );
	    dbc.write( JDBC_XP_XA_BQUAL );
	    dbc.write( xid.getBranchQualifier() );
	    break;
	}

	dbc.done( true );
	readResults();
    }
    catch( EdbcEx ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".startTransaction(): error starting xact" );
	if ( trace.enabled( 1 ) )  ex.trace( trace );
	throw ex;
    }
    finally 
    { 
	dbc.unlock(); 
    }

    return;
} // startTransaction


/*
** Name: prepareTransaction
**
** Description:
**	Prepare a distributed transaction.  A call to startTransaction(),
**	with no intervening commit() or rollback() call, must already
**	have been made.
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
**	21-Mar-01 (gordy)
**	    Created.
**	28-Mar-01 (gordy)
**	    Check protocol for server support.
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
*/

public void
prepareTransaction()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".prepareTransaction()" );
    warnings = null;

    if ( dtmc )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": not permitted when DTMC" );
	throw EdbcEx.get( E_JD0012_UNSUPPORTED );
    }

    if ( dbc.msg_protocol_level < JDBC_MSG_PROTO_2 )
    {
	if ( trace.enabled( 1 ) ) 
	    trace.write( tr_id + ": protocol = " + dbc.msg_protocol_level );
	throw EdbcEx.get( E_JD0012_UNSUPPORTED );
    }

    dbc.lock();
    try
    {
	dbc.begin( JDBC_MSG_XACT );
	dbc.write( JDBC_XACT_PREPARE );
	dbc.done( true );
	readResults();
    }
    catch( EdbcEx ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".prepareTransaction(): error preparing xact" );
	if ( trace.enabled( 1 ) )  ex.trace( trace );
	throw ex;
    }
    finally 
    { 
	dbc.unlock(); 
    }

    return;
} // prepareTransaction


/*
** Name: getPreparedTransactionIDs
**
** Description:
**	Returns a list of distributed transaction IDs which
**	are active for the database specified.  The connection
**	must be for the installation master database 'iidbdb'.
**
** Input:
**	db	Database name.
**
** Output:
**	None.
**
** Returns:
**	EdbcXID[]   Transaction IDs.
**
** History:
**	18-Apr-01 (gordy)
**	    Created.
*/

public EdbcXID[]
getPreparedTransactionIDs( String db )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getPreparedTransactionIDs('" + db +"')" );
    warnings = null;

    if ( dbc.msg_protocol_level >= JDBC_MSG_PROTO_2 )
    {
	/*
	** Request the info from the JDBC Server.
	*/
	if ( dbXids == null )  dbXids = new EdbcXids( dbc, trace );
	return( dbXids.readXids( db ) );
    }

    return( new EdbcXID[ 0 ] );
} // getPreparedTransactionIDs


/*
** Name: getDbmsInfo
**
** Description:
**	Cover method which restricts requests to cached values.
**
** Input:
**	id	ID or Name of desired info.
**
** Output:
**	None.
**
** Returns:
**	String	Value associated with ID, or NULL.
**
** History:
**	25-Oct-01 (gordy)
**	    Created.
*/

String
getDbmsInfo( String id )
    throws SQLException
{
    return( getDbmsInfo( id, true ) );
} // getDbmsInfo


/*
** Name: getDbmsInfo
**
** Description:
**	Combines the information obtained about the DBMS 
**	when connection established (capabilities loaded
**	by readDBCaps()) and the additional information
**	available through the SQL function dbmsinfo().
**
** Input:
**	id	    ID or Name of desired info.
**	cache_only  Restrict to currently cached items.
**
** Output:
**	None.
**
** Returns:
**	String	Value associated with ID, or NULL.
**
** History:
**	 7-Mar-01 (gordy)
**	    Created.
**	16-Apr-01 (gordy)
**	    DBMS info now returned by EdbcDBInfo.readDBInfo().
**	25-Oct-01 (gordy)
**	    Added cache_only parameter so as to avoid issuing
**	    server request for items loaded when connection
**	    started.  Make sure 'select dbmsinfo()' query is
**	    readonly.
*/

String
getDbmsInfo( String id, boolean cache_only )
    throws SQLException
{
    String  val = dbc.getDbmsInfo( id );    // Check for Cached value.
    if ( cache_only  ||  val != null )  return( val );

    if ( dbc.msg_protocol_level >= JDBC_MSG_PROTO_2 )
    {
	/*
	** Request the info from the JDBC Server.
	*/
	if ( dbInfo == null )  dbInfo = new EdbcDBInfo( dbc, trace );
	val = dbInfo.readDBInfo( id );
    }
    else
    {
	/*
	** Use a standard JDBC statement to query the info.
	*/
	try
	{
	    ResultSet   rs;
	    Statement   stmt = createStatement( ResultSet.TYPE_FORWARD_ONLY,
						ResultSet.CONCUR_READ_ONLY );
	    String	query = "select dbmsinfo('" + id + "')";

	    if ( ! dbc.is_ingres )  query += " from iidbconstants";
    
	    rs = stmt.executeQuery( query );
	    if ( rs.next() ) val = rs.getString(1);
	    rs.close();
	    stmt.close();
	}
	catch( SQLException ex )
	{
	    if ( trace.enabled( 1 ) )  
	    {
		trace.log( title + ".getDbmsInfo(): failed!" ) ;
		((EdbcEx)ex).trace( trace );
	    }
	    throw ex;
	}
    }

    /*
    ** Cache any value retrieved.
    */
    if ( val != null  &&  val.length() > 0 )  dbc.setDbmsInfo( id, val );
    return( val );
}


/*
** Name: EdbcDBInfo
**
** Description:
**	Class which implements the ability to load the DBMS information.
**
**
**  Public Methods:
**
**	readDBInfo		Read DBMS information.
**
**  Private Data:
**
**	rsmd			Result-set Meta-data.
**	info			DBMS information.
**
**  Private Methods:
**
**	readDesc		Read data descriptor.
**	readData		Read data.
**
** History:
**	 7-Mar-01 (gordy)
**	    Created as extract from EdbcConnect.
**	16-Apr-01 (gordy)
**	    Since the result-set is pre-defined, maintain a single RSMD.
**	    DBMS Information is now returned rather than saving in DbConn.
*/

private static class
EdbcDBInfo
    extends EdbcObj
{

    private static EdbcRSMD	rsmd = null;
    private String		info;


/*
** Name: EdbcDBInfo
**
** Description:
**	Class constructor.
**
** Input:
**	dbc	Database connection.
**	trace	Connection tracing.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 7-Mar-01 (gordy)
**	    Created.
**	28-Mar-01 (gordy)
**	    Added tracing parameter.
*/

public
EdbcDBInfo( DbConn dbc, EdbcTrace trace )
{
    super( dbc, trace );
}


/*
** Name: readDBInfo
**
** Description:
**	Query DBMS for information.
**
** Input:
**	id	ID or Name of DBMS information.
**
** Output:
**	None.
**
** Returns:
**	String	DBMS information.
**
** History:
**	 7-Mar-01 (gordy)
**	    Created.
**	16-Apr-01 (gordy)
**	    Return DBMS information.
*/

public synchronized String
readDBInfo( String id )
    throws EdbcEx
{
    info = null;
    dbc.lock();
    try
    {
	dbc.begin( JDBC_MSG_REQUEST );
	dbc.write( JDBC_REQ_DBMSINFO );
	dbc.write( JDBC_RQP_INFO_ID );
	dbc.write( id );
	dbc.done( true );
	readResults();
    }
    catch( EdbcEx ex )
    {
	if ( trace.enabled( 1 ) )  trace.log( "readDBInfo(): failed!" );
	if ( trace.enabled( 1 ) )  ex.trace( trace );
	throw ex;
    }
    finally
    {
	dbc.unlock();
    }
    return( info );
} // readDBInfo


/*
** Name: readDesc
**
** Description:
**	Read a data descriptor message.  Overrides 
**	default method in EdbcObj.  It handles the
**	reading of descriptor messages for the method
**	readDBInfo (above).
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	EdbcRSMD    Data descriptor.
**
** History:
**	  7-Mar-01 (gordy)
**	    Created.
**	16-Apr-01 (gordy)
**	    Instead of creating a new RSMD on every invocation,
**	    use load() and reload() methods of EdbcRSMD for just
**	    one RSMD.  Return the RSMD, caller can ignore or use.
*/

protected EdbcRSMD
readDesc()
    throws EdbcEx
{
    if ( rsmd == null )
	rsmd = EdbcRSMD.load( dbc, trace );
    else
	EdbcRSMD.reload( dbc, rsmd );
    return( rsmd );
} // readDesc


/*
** Name: readData
**
** Description:
**	Read a data message.  This method overrides the readData
**	method of EdbcObj and is called by the readResults
**	method.  It handles the reading of data messages for the
**	readDBInfo() method (above).  The data should be in a
**	pre-defined format (row containing 1 non-null varchar 
**	column).
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    Interrupt reading results.
**
** History:
**	 7-Mar-01 (gordy)
**	    Created.
**	16-Apr-01 (gordy)
**	    Save info result locally rather than in DbConn.
*/

protected boolean
readData()
    throws EdbcEx
{
    if ( dbc.moreData() )
    {
	dbc.readByte();			// NULL byte (never null)
	info = dbc.readString();	// Information value.
    }

    return( false );
} // readData

} // class EdbcDBInfo

	
/*
** Name: EdbcDBCaps
**
** Description:
**	Class which implements the ability to load the DBMS
**	capabilities into the DbConn.
**
**  Public Methods:
**
**	readDBCaps		Read DBMS capabilities.
**
**  Private Data:
**
**	rsmd			Result-set Meta-data.
**
**  Private Methods:
**
**	readDesc		Read data descriptor.
**	readData		Read data.
**
** History:
**	 7-Mar-01 (gordy)
**	    Created as extract from EdbcConnect.
**	16-Apr-01 (gordy)
**	    Since the result-set is pre-defined, maintain a single RSMD.
*/

private static class
EdbcDBCaps
    extends EdbcObj
{

    private static EdbcRSMD	rsmd = null;


/*
** Name: EdbcDBCaps
**
** Description:
**	Class constructor.
**
** Input:
**	dbc	Database connection.
**	trace	Connection tracing.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 7-Mar-01 (gordy)
**	    Created.
**	28-Mar-01 (gordy)
**	    Added tracing parameter.
*/

public
EdbcDBCaps( DbConn dbc, EdbcTrace trace )
{
    super( dbc, trace );
}


/*
** Name: readDBCaps
**
** Description:
**	Query DBMS for capabilities.  Results stored in DbConn.
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
**	 2-Nov-99 (gordy)
**	    Created.
**	12-Nov-99 (gordy)
**	    If OPEN_SQL_DATES, use local time rather than gmt.
**	17-Dec-99 (gordy)
**	    Add 'for readonly' now that cursor pre-fetch is supported.
**	17-Jan-00 (gordy)
**	    Replaced iidbcapabilities query with a new REQUEST message
**	    to execute a pre-defined server query/statement.
**	 3-Nov-00 (gordy)
**	    Timezone stuff removed from DbConn.  Replaced with a public 
**	    boolean indicating which timezone to use for the connection.
**	 7-Mar-01 (gordy)
**	    Extracted and renamed.
**	10-May-01 (gordy)
**	    Moved capability check to a more relevent method.
*/

public void
readDBCaps()
    throws EdbcEx
{
    dbc.lock();
    try
    {
	dbc.begin( JDBC_MSG_REQUEST );
	dbc.write( JDBC_REQ_CAPABILITY );
	dbc.done( true );
	readResults();
    }
    catch( EdbcEx ex )
    {
	if ( trace.enabled( 1 ) )  
	    trace.log( title + ": error loading DBMS capabilities" );
	throw ex;
    }
    finally
    {
	dbc.unlock();
    }
    return;
} // readDBCaps


/*
** Name: readDesc
**
** Description:
**	Read a data descriptor message.  Overrides 
**	default method in EdbcObj.  It handles the
**	reading of descriptor messages for the method
**	readDBCaps (above).
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	EdbcRSMD    Data descriptor.
**
** History:
**	13-Jun-00 (gordy)
**	    Created.
**	16-Apr-01 (gordy)
**	    Instead of creating a new RSMD on every invocation,
**	    use load() and reload() methods of EdbcRSMD for just
**	    one RSMD.  Return the RSMD, caller can ignore or use.
*/

protected EdbcRSMD
readDesc()
    throws EdbcEx
{
    if ( rsmd == null )
	rsmd = EdbcRSMD.load( dbc, trace );
    else
	EdbcRSMD.reload( dbc, rsmd );
    return( rsmd );
} // readDesc


/*
** Name: readData
**
** Description:
**	Read a data message.  This method overrides the readData
**	method of EdbcObj and is called by the readResults
**	method.  It handles the reading of data messages for the
**	readDBCaps() method (above).  The data should be in a
**	pre-defined format (rows containing 2 non-null varchar 
**	columns).
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    Interrupt reading results.
**
** History:
**	17-Jan-00 (gordy)
**	    Created.
*/

protected boolean
readData()
    throws EdbcEx
{
    while( dbc.moreData() )
    {
	String s1, s2;
	
	dbc.readByte();		// NULL byte (never null)
	s1 = dbc.readString();	// Capability name
	dbc.readByte();		// NULL byte (never null)
	s2 = dbc.readString();	// Capability value

	dbc.setDbmsInfo( s1, s2 );
    }

    return( false );
} // readData

} // class EdbcDBCaps


/*
** Name: EdbcXids
**
** Description:
**	Class which implements the ability to read prepared
**	transaction IDs.
**
**
**  Public Methods:
**
**	readXids		Read transaction IDs.
**
**  Private Data:
**
**	rsmd			Result-set Meta-data.
**	xids			XID list.
**
**  Private Methods:
**
**	readDesc		Read data descriptor.
**	readData		Read data.
**
** History:
**	18-Apr-01 (gordy)
**	    Created.
*/

private static class
EdbcXids
    extends EdbcObj
{

    private static EdbcRSMD	rsmd = null;
    private Vector		xids = new Vector();


/*
** Name: EdbcXids
**
** Description:
**	Class constructor.
**
** Input:
**	dbc	Database connection.
**	trace	Connection tracing.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	18-Apr-01 (gordy)
**	    Created.
*/

public
EdbcXids( DbConn dbc, EdbcTrace trace )
{
    super( dbc, trace );
}


/*
** Name: readXids
**
** Description:
**	Query DBMS for transaction IDs.
**
** Input:
**	db	    Database name.
**
** Output:
**	None.
**
** Returns:
**	EdbcXID[]   Transaction IDs.
**
** History:
**	18-Apr-01 (gordy)
**	    Created.
*/

public synchronized EdbcXID[]
readXids( String db )
    throws EdbcEx
{
    xids.clear();
    dbc.lock();
    try
    {
	dbc.begin( JDBC_MSG_REQUEST );
	dbc.write( JDBC_REQ_2PC_XIDS );
	dbc.write( JDBC_RQP_DB_NAME );
	dbc.write( db );
	dbc.done( true );
	readResults();
    }
    catch( EdbcEx ex )
    {
	if ( trace.enabled( 1 ) )  trace.log( "readXids(): failed!" );
	if ( trace.enabled( 1 ) )  ex.trace( trace );
	throw ex;
    }
    finally
    {
	dbc.unlock();
    }

    /*
    ** XIDs have been collected in xids.
    ** Allocate and populate the XID array.
    */
    EdbcXID xid[] = new EdbcXID[ xids.size() ];

    for( int i = 0; i < xid.length; i++ )
	xid[ i ] = (EdbcXID)xids.get( i );

    xids.clear();
    return( xid );
} // readXids


/*
** Name: readDesc
**
** Description:
**	Read a data descriptor message.  Overrides 
**	default method in EdbcObj.  It handles the
**	reading of descriptor messages for the method
**	readXids (above).
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	EdbcRSMD    Data descriptor.
**
** History:
**	18-Apr-01 (gordy)
**	    Created.
*/

protected EdbcRSMD
readDesc()
    throws EdbcEx
{
    if ( rsmd == null )
	rsmd = EdbcRSMD.load( dbc, trace );
    else
	EdbcRSMD.reload( dbc, rsmd );
    return( rsmd );
} // readDesc


/*
** Name: readData
**
** Description:
**	Read a data message.  This method overrides the readData
**	method of EdbcObj and is called by the readResults
**	method.  It handles the reading of data messages for the
**	readXids() method (above).  The data should be in a
**	pre-defined format (rows containing 3 non-null columns
**	of type int, byte[], byte[]).
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    Interrupt reading results.
**
** History:
**	18-Apr-01 (gordy)
**	    Created.
*/

protected boolean
readData()
    throws EdbcEx
{
    while( dbc.moreData() )
    {
	dbc.readByte();			// Format ID - NULL byte (never null)
	int fid = dbc.readInt();
	
	dbc.readByte();			// Global Transaction ID
	byte gtid[] = dbc.readBytes();
	
	dbc.readByte();			// Branch Qualifier
	byte bqual[] = dbc.readBytes();

	xids.add( new EdbcXID( fid, gtid, bqual ) );
    }

    return( false );
} // readData

} // class EdbcXids

} // class EdbcConnect
