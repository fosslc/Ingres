/*
** Copyright (c) 1999, 2010 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: JdbcConn.java
**
** Description:
**	Defines class which implements the JDBC Connection interface.
**
**  Classes
**
**	JdbcConn	Implements the JDBC Connection interface.
**	DbXids		Access registered XIDs.
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
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
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
**      30-sep-02 (loera01) Bug 108833
**          Since the intention of the setTransactionIsolaton and
**          setReadOnly methods is for the "set" query to persist in the
**          session, change "set transaction" to "set session" in the
**          sendTransactionQuery private method.
**	01-Oct-02 (wansh01)
**	    Added support for MSG_CP_LOGIN_TYPE as a connection parameter. 	
**	31-Oct-02 (gordy)
**	    Extracted non JDBC Connection interface code to new class DrvConn.
**	06-Dec-02 (wansh01)
**	    Added support for optional UserId for local connection. 
** 	    Added checkLocalUser(). for local connection, User ID is 
**	    optional.  If no userId is specified. the current system 
**	    sign on id is used and MSG_CP_LOGIN_TYPE is set to 
**	    MSG_CPV_LOGIN_USER. 
**	23-Dec-02 (gordy)
**	    New messaging (DAM-ML) protocol level and connection parameter.
**	15-Jan-03 (gordy)
**	    Added client information connection parameters.
**	14-Feb-03 (gordy)
**	    Updated to JDBC 3.0.
**	15-Apr-03 (gordy)
**	    Added connection timezones separate from date formatters.
**	 7-Jul-03 (gordy)
**	    Add Ingres configuration connection parameters.
**	16-Jun-04 (gordy)
**	    Added session_mask to password encryption.
**	20-Dec-04 (gordy)
**	    Clear prepared statement cache when rolling back to savepoint.
**	 2-Feb-05 (gordy)
**	    Added ability to forcefully abort connection.
**	10-May-05 (gordy)
**	    Added ability to abort transaction via independent connection.
**	19-Jun-06 (gordy)
**	    ANSI Date/Time data type support.
**	 3-Jul-06 (gordy)
**	    Processing encoding driver property.  Verify connection encoding.
**	20-Jul-06 (gordy)
**	    Extend XA support.
**	6-Apr-07 (gordy)
**	    Support scrollable cursors.
**	 4-May-07 (gordy)
**	    Set class access for reflection.
**	23-Jul-07 (gordy)
**	    Added connection parameter for date alias.
**      05-Jan-09 (rajus01) SIR 121238
**          - Added new JDBC 4.0 methods to avoid compilation errors with
**            JDK 1.6. The new methods return E_GC4019 error for now.
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to
**            support JDBC 4.0 SQLException hierarchy.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**      11-Feb-10 (rajus01) Bug 123277
**	    Check for 'send_ingres_dates' connection parameter (internal). 
**	30-Jul-10 (gordy)
**	    Check for 'send_integer_booleans' connection parameter (internal). 
**	10-Nov-10 (gordy)
**	    Using the DAS default for the date alias property when
**	    the send_ingres_dates property is set to false results
**	    in the conflicting behavior of ANSI date/time values
**	    being sent to the server when the DATE keyword is being
**	    interpreted as "ingresdate".  The date alias default
**	    should be determined from the send_ingres_dates setting.
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
import	java.sql.Savepoint;
import	java.sql.SQLException;
import  java.sql.Blob;
import  java.sql.Clob;
import  java.sql.Array;
import  java.sql.NClob;
import  java.sql.SQLXML;
import  java.sql.Struct;
import  java.sql.SQLClientInfoException;
import  java.sql.ClientInfoStatus;
import  java.util.Properties;
import  java.util.Map;
import  java.util.HashMap;
import  java.util.Iterator;
import  java.sql.Wrapper;
import  java.util.Properties;
import	com.ingres.gcf.util.BufferedBlob;
import	com.ingres.gcf.util.BufferedClob;
import	com.ingres.gcf.util.BufferedNlob;
import	com.ingres.gcf.util.CharSet;
import	com.ingres.gcf.util.Config;
import	com.ingres.gcf.util.DbmsConst;
import	com.ingres.gcf.util.IdMap;
import	com.ingres.gcf.util.GcfErr;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.util.SqlWarn;
import	com.ingres.gcf.util.XaEx;
import	com.ingres.gcf.util.IngXid;
import	com.ingres.gcf.util.XaXid;


/*
** Name: JdbcConn
**
** Description:
**	JDBC driver class which implements the JDBC Connection interface.
**
**  Interface Methods:
**
**	isClosed		Determine if connection is closed.
**	close			Close the connection.
**	createArrayOf		Create Array object.
**	createBlob		Create Blob object.
**	createClob		Create Clob object.
**	createNClob		Create NClob object.
**	createSQLXML		Create SQLXML object.
**	createStatement		Create a Statement object.
**	createStruct		Create a Struct object.
**	prepareStatement	Create a PreparedStatement object.
**	prepareCall		Create a CallableStatement object.
**	getHoldability		Retrieve ResultSet holdability.
**	setHoldability		Set ResultSet holdability.
**	nativeSQL		Translate SQL into native DBMS SQL.
**	getMetaData		Create a DatabaseMetaData object.
**	getCatalog		Retrieve the DBMS catalog.
**	setCatalog		Set the DBMS catalog.
**	setClientInfo		Set the value of client info property.
**	getClientInfo		Retrieve the value of client info property.
**	getTypeMap		Retrieve the type map.
**	setTypeMap		Set the type map.
**	isReadOnly		Determine readonly state.
**	isValid			Determine connection validity.
**	isWrapperFor		Does it wrap an object the implements the iface?
**	unwrap			Retrieves the object that implements the iface.
**	setReadOnly		Set readonly state
**	getTransactionIsolation	Determine the transaction isolation level.
**	setTransactionIsolation	Set the transaction isolation level.
**	getAutoCommit		Determine autocommit state.
**	setAutoCommit		Set autocommit state.
**	commit			Commit current transaction.
**	rollback		Rollback current transaction.
**	setSavepoint		Create savepoint.
**	releaseSavepoint	Remove savepoint.
**
**  Public Methods:
**
**	startTransaction	Start distributed transaction.
**	endTransaction		End distributed transaction.
**	prepareTransaction	Prepare distributed transaction.
**	getPreparedTransactionIDs   Returns IDs of prepared transactions.
**
**  Private Data
**
**	isolationLevel		Current transaction isolationLevel.
**	savepoints		Savepoint chain.
**	dbmd			Database Meta-data
**	type_map		Map for DBMS datatypes.
**	dbXids			Distributed XID access.
**
**  Private Methods:
**
**	connect			Establish DBMS connection.
**	checkLocalUser		Determine if local user ID should be used.
**	client_parms		Process client provided connection parameters.
**	driver_parms		Process internal driver connection parameters.
**	createStmt		Create statement object.
**	createPrep		Create prepared statement object.
**	createCall		Create callable statement object.
**	sendTransactionQuery	Send 'SET TRANSACTION' query to DBMS.
**	createSavepoint		Send savepoint request to server.
**	clearSavepoints		Clear savepoint chain.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
**	 8-Sep-99 (gordy)
**	    Created new base class for class which interact with
**	    the JDBC server and extracted common data and methods.
**	    Synchronize entire request/response with JDBC server.
**	30-Oct-00 (gordy)
**	    Support JDBC 2.0 extensions.  Added type_map, getTypeMap(),
**	    setTypeMap(), createStatement(), prepareStatement(),
**	    prepareCall(), and getDbConn().  Removed getDbmsInfo().
**	21-Mar-01 (gordy)
**	    Added support for distributed transactions with methods
**	    startTransaction() and prepareTransaction().
**	28-Mar-01 (gordy)
**	    Dropped getDbConn() as dbc now has package access.
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
**	    Added getPreparedTransactionIDs() method and constructor which
**	    establishes a DTMC.  Added dtmc flag and EdbcXids request class.
**	31-Oct-02 (gordy)
**	    Extracted non JDBC Connection interface code to new class DrvConn.
**	    Removed OI_SQL_LEVEL, host, database, dtmc, autoCommit, readOnly,
**	    dbInfo fields; server_connect(), disconnect(), getDbmsInfo()
**	    methods; DB info and DB caps classes.  Added dbmd to maintain 
**	    single meta-data object per connection.  Renamed dbms_connect() 
**	    to connect() and extracted parameter processing to connect_parms().
**	06-Dec-02 (wansh01)
**	    Added support for optional userid for local connection.
**	15-Jan-03 (gordy)
**	    Separated internal connection parameters from connect() into
**	    driver_parms().  Renamed connect_parms() to client_parms().
**	14-Feb-03 (gordy)
**	    Added support for savepoints: savepoints, holdability,
**	    rollback( savepoint ), setSavepoint(), releaseSavepoint(),
**	    createSavepoint(), clearSavepoints(), getHoldability(),
**	    setHoldability().
**	 2-Feb-05 (gordy)
**	    Added abort() to abort connection.
**	10-May-05 (gordy)
**	    Added getTransactionServer() and abortTransaction() to
**	    enabled transaction abort via independent DTM connection.
**	20-Jul-06 (gordy)
**	    Added endTransaction() and additional transaction methods
**	    to support enhanced XA server capabilities.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, permit access.
*/

public class
JdbcConn
    extends	DrvObj
    implements	Connection, DbmsConst
{

    private int		isolationLevel	= TRANSACTION_SERIALIZABLE;
    private int		holdability	= ResultSet.CLOSE_CURSORS_AT_COMMIT;
    private JdbcSP	savepoints	= null;
    private JdbcDBMD	dbmd		= null;
    private Map		type_map	= new Hashtable();
    private DbXids	dbXids		= null;


/*
** Name: JdbcConn
**
** Description:
**	Class constructor.  Establish a DBMS connection.  The
**	connection timeout value is interpreted as follows:
**
**	positive    Timeout in seconds.
**	0	    No timeout (full blocking).
**	negative    Timeout in milli-seconds.
**
** Input:
**	conn	Database connection.
**	config	Configuration properties.
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
**	 2-Nov-99 (gordy)
**	    Read DBMS information after establishing connection.
**	28-Mar-01 (gordy)
**	    Moved server connection establishment into this class and
**	    placed server and database connection code into separate
**	    methods.
**	11-Apr-01 (gordy)
**	    Added timeout parameter for dbms connection.
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 7-Jul-03 (gordy)
**	    Changed properties parameter to support config heirarchy.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, restrict constructor access.
*/

// package access
JdbcConn( DrvConn conn, Config config, int timeout )
    throws SQLException
{
    this( conn );
    connect( config, timeout, null, null );
    conn.loadDbCaps();
    return;
} // JdbcConn


/*
** Name: JdbcConn
**
** Description:
**	Class constructor.  Establish a DBMS connection
**	associated with an existing distribute transaction.
**
** Input:
**	conn	Database connection.
**	config	Configuration properties.
**	xid	Ingres distributed transaction ID.
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
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 7-Jul-03 (gordy)
**	    Changed properties parameter to support config heirarchy.
*/

// package access
JdbcConn( DrvConn conn, Config config, IngXid xid )
    throws SQLException
{
    this( conn );
    connect( config, 0, xid, null );
    return;
} // JdbcConn


/*
** Name: JdbcConn
**
** Description:
**	Class constructor.  Establish a DBMS connection
**	associated with an existing distributed transaction.
**
** Input:
**	conn	Database connection.
**	config	Configuration properties.
**	xid	XA distributed transaction ID.
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
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 7-Jul-03 (gordy)
**	    Changed properties parameter to support config heirarchy.
*/

// package access
JdbcConn( DrvConn conn, Config config, XaXid xid )
    throws SQLException
{
    this( conn );
    connect( config, 0, null, xid );
    return;
} // JdbcConn


/*
** Name: JdbcConn
**
** Description:
**	Class constructor.  Common initialization.
**
** Input:
**	conn	Database connection.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 4-Oct-00 (gordy)
**	    Create unique ID for standardized internal tracing.
**	18-Apr-01 (gordy)
**	    Extracted from other constructors.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
*/

private
JdbcConn( DrvConn conn )
{
    super( conn );
    conn.jdbc = this;
    title = trace.getTraceName() + "-Connection[" + msg.connID() + "]";
    tr_id = "Conn[" + msg.connID() + "]";
    return;
} // JdbcConn


/*
** Name: connect
**
** Description:
**	Establish connection with DBMS.
**
** Input:
**	config	Configuration properties.
**	timeout	Connection timeout.
**	ing_xid	Ingres distributed transaction ID.
**	xa_xid	XA distributed transaction ID.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 4-Nov-99 (gordy)
**	    Send timeout connection parameter to server.
**	28-Mar-01 (gordy)
**	    Separated from constructor.
**	11-Apr-01 (gordy)
**	    Added timeout parameter to replace DriverManager reference.
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
**	31-Oct-02 (gordy)
**	    Renamed and extracted parameter processing to connect_parms().
**	    DBMS info processing moved to DrvConn class.
**	06-Dec-02 (wansh01)
**	    Added support for optional userid for local connection.
**	15-Jan-03 (gordy)
**	    Extracted internal connection parameters to driver_parms().
**	 7-Jul-03 (gordy)
**	    Changed properties parameter to support config heirarchy.
**	 3-Jul-06 (gordy)
**	    Check connection character encoding and property override.
*/

private void
connect( Config config, int timeout, IngXid ing_xid, XaXid xa_xid )
    throws SQLException
{
    /*
    ** Check if character encoding has been established
    ** for the connection.  If not, it may be explicitly
    ** set via driver properties.  Encoding must be set
    ** before connection parameters are processed.
    */
    String  encode = config.get( DRV_PROP_ENCODE );

    if ( encode != null  &&  encode.length() != 0 )
    {
	try { msg.setCharSet( new CharSet( encode ) ); }
	catch( Exception ex )
	{
	    if ( trace.enabled( 1 ) )
		trace.write( tr_id + ": unknown encoding: " + encode );
	    throw SqlExFactory.get( ERR_GC4009_BAD_CHARSET );
	}
    }
    else  if ( msg.getCharSet() == null )
    {
	if ( trace.enabled() )
	    trace.log(title + ": failed to establish character encoding!");
	throw SqlExFactory.get( ERR_GC4009_BAD_CHARSET );
    }

    if ( trace.enabled(2) )  trace.write( tr_id + ": Connecting to database" ); 

    try
    {
	msg.begin( MSG_CONNECT );
	client_parms( config );
	driver_parms( checkLocalUser( config ), timeout, ing_xid, xa_xid );
	msg.done( true );
	readResults();
    }
    catch( SQLException ex ) 
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( tr_id + ": error connecting to database" );
	conn.close();
	throw ex;
    }

    if ( trace.enabled(2) )  trace.write( tr_id + ": connected to database" );
    return;
} // connect



/*
** Name: checkLocalUser
**
** Description:
**	Check to see if local user ID should be used to establish DBMS
**	connection.  Any application provided user ID is used instead.
**	The connection must be to a local server and not DTMC.
**
** Input:
**	config	Configuration properties.
**
** Output:
**	None.
**
** Returns:
**	boolean	TRUE if local user ID should be used, FALSE otherwise.
**
** History:
**	31-Oct-02 (wansh01)
**	    Created. 
**	23-Dec-02 (gordy)
**	    Check protocol level.
**	 7-Jul-03 (gordy)
**	    Changed properties parameter to support config heirarchy.
**	    Connection parameters now set in driver_parms.  Don't need
**	    to see if local user ID is accessible, it will be handled
**	    when trying to send the user ID.
*/

private boolean
checkLocalUser( Config config )
{
    String	userID;

    return( msg.isLocal()  &&  ! conn.is_dtmc  &&  
	    conn.msg_protocol_level >= MSG_PROTO_3  &&
	    ( (userID = config.get( DRV_PROP_USR )) == null  ||  
	      userID.length() == 0 ) );
} // checkLocalUser


/*
** Name: client_parms
**
** Description:
**	Process driver connection properties and send DAM-ML 
**	connection parameters.
**
** Input:
**	config	Configuration properties.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	17-Sep-99 (rajus01)
**	    Added connection parameters.
**	22-Sep-99 (gordy)
**	    Convert user/password to server character set when encrypting.
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
**	17-Oct-00 (gordy)
**	    Added autocommit mode connection parameter.
**	20-Jan-01 (gordy)
**	    Autocommit mode only supported at protocol level 2 or higher.
**	28-Mar-01 (gordy)
**	    Separated from constructor.
**	20-Aug-01 (gordy)
**	    Added connection parameter for default cursor mode.
**	01-Oct-02 (wansh01)
**	    Added support for MSG_CP_LOGIN_TYPE as a connection parameter. 	
**	31-Oct-02 (gordy)
**	    Extracted from connect().
**	23-Dec-02 (gordy)
**	    Check protocol level for login type.  Added (no-op) CHAR_ENCODE.
**	18-Feb-03 (gordy)
**	    Property login type changed to vnode usage and changed values:
**	    LOCAL -> LOGIN, REMOTE -> CONNECT.  Message parameter unchanged.
**	 7-Jul-03 (gordy)
**	    Add Ingres configuration parameters.  Changed properties 
**	    parameter to support config heirarchy.
**	16-Jun-04 (gordy)
**	    Added session_mask to password encryption.
**	23-Jul-07 (gordy)
**	    Added connection parameter for date alias.
**      11-Feb-10 (rajus01) Bug 123277
**          Added connection parameter for 'send_ingres_dates'.
**	30-Jul-10 (gordy)
**	    Added connection parameter for 'send_integer_booleans'.
**	    Converted config item snd_ing_dte to config flag.
**	10-Nov-10 (gordy)
**	    Made defaults for date_alias and send_ingres_dates consistent.
**	    If neither is set, ANSI dates are used.  If one is set, the
**	    other is default to a similar setting.
*/

private void
client_parms( Config config )
    throws SQLException
{
    boolean	date_alias_set = false;
    boolean	send_ingdate_set = false;
    boolean	use_ingres_dates = false;
    byte	i1;
    
    if ( trace.enabled( 3 ) )  
	trace.write( tr_id + ": sending connection parameters" );

    for( int i = 0; i < propInfo.length; i++ )
    {
	String value = config.get( propInfo[ i ].name );
	if ( value == null  ||  value.length() == 0 )  continue;
	
	switch( propInfo[ i ].msgID )
	{
	case MSG_CP_DATABASE :
	    /*
	    ** Save parameter value for meta-data.
	    */
	    conn.database = value;
	    msg.write( propInfo[ i ].msgID );
	    msg.write( value );
	    break;

	case MSG_CP_PASSWORD :
	{
	    /*
	    ** Encrypt password using user ID (Don't send if user ID
	    ** not provided.
	    */
	    String user = config.get( DRV_PROP_USR );
	    
	    if ( user != null  &&  user.length() > 0 )  
	    {
	        msg.write( propInfo[ i ].msgID );
	        msg.write( msg.encode( user, conn.session_mask, value ) );
	    }
		
	    value = "*****";		// Hide password from tracing.
	    break;
	}

 	case MSG_CP_DBPASSWORD :
 	    msg.write( propInfo[ i ].msgID );
 	    msg.write( value );
	    value = "*****";		// Hide password from tracing.
 	    break;

	case MSG_CP_CONNECT_POOL :
	    /*
	    ** Convert to symbolic value.
	    */
	    if ( value.equalsIgnoreCase( "off" ) )
		i1 = MSG_CPV_POOL_OFF;
	    else  if ( value.equalsIgnoreCase( "on" ) )
		i1 = MSG_CPV_POOL_ON;
	    else
	    {
		if ( trace.enabled( 1 ) ) 
		    trace.write( tr_id + ": connection pool '" + value + "'" );
		throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
	    }
	    
	    msg.write( propInfo[ i ].msgID );
	    msg.write( (short)1 );
	    msg.write( i1 );
	    break;

	case MSG_CP_AUTO_MODE :
	    /*
	    ** Convert to symbolic value.  Only default setting
	    ** permitted at lower protocol levels and param is
	    ** not sent.
	    */
	    if ( value.equalsIgnoreCase( "dbms" ) )
		i1 = MSG_CPV_XACM_DBMS;
	    else  if ( value.equalsIgnoreCase( "single" ) )
		i1 = MSG_CPV_XACM_SINGLE;
	    else  if ( value.equalsIgnoreCase( "multi" ) )
		i1 = MSG_CPV_XACM_MULTI;
	    else
	    {
		if ( trace.enabled( 1 ) ) 
		    trace.write( tr_id + ": autocommit mode '" + value + "'" );
		throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
	    }
	    
	    if ( conn.msg_protocol_level >= MSG_PROTO_2 )
	    {
		msg.write( propInfo[ i ].msgID );
		msg.write( (short)1 );
		msg.write( i1 );
	    }
	    else  if ( i1 != MSG_CPV_XACM_DBMS )
	    {
		if ( trace.enabled( 1 ) ) 
		    trace.write( tr_id + ": autocommit mode '" + value +
				 "' @ proto lvl " + conn.msg_protocol_level );
		throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
	    }
	    break;

	case MSG_CP_LOGIN_TYPE :
	    /*
	    ** Convert to symbolic value.  Only default setting
	    ** permitted at lower protocol levels and param is
	    ** not sent.
	    */
	    if ( value.equalsIgnoreCase( "login" ) )
		i1 = MSG_CPV_LOGIN_LOCAL;
	    else  if ( value.equalsIgnoreCase( "connect" ) )
		i1 = MSG_CPV_LOGIN_REMOTE;
	    else
	    {
		if ( trace.enabled( 1 ) ) 
		    trace.write( tr_id + ": vnode usage '" + value + "'" );
		throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
	    }
	    
	    if ( conn.msg_protocol_level >= MSG_PROTO_3 )
	    {
		msg.write( propInfo[ i ].msgID );
		msg.write( (short)1 );
		msg.write( i1 );
	    }
	    else  if ( i1 != MSG_CPV_LOGIN_REMOTE )
	    {
		if ( trace.enabled( 1 ) ) 
		    trace.write( tr_id + ": vnode usage '" + value + 
				 "' @ proto lvl " + conn.msg_protocol_level );
		throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
	    }
	    break;

	case MSG_CP_TIMEZONE :
	    /*
	    ** Ignored at lower protocol levels to permit setting
	    ** in property file for all connections.
	    */
	    if ( conn.msg_protocol_level >= MSG_PROTO_3 )
	    {
		/*
		** Note that Ingres timezone has been sent to DBMS.
		*/
		conn.is_gmt = false;
		msg.write( propInfo[ i ].msgID );
		msg.write( value );
	    }
	    break;
	    
	    
	case MSG_CP_DECIMAL :
	case MSG_CP_DATE_FRMT :
	case MSG_CP_MNY_FRMT :
	    /*
	    ** Ignored at lower protocol levels to permit setting
	    ** in property file for all connections.
	    */
	    if ( conn.msg_protocol_level >= MSG_PROTO_3 )
	    {
		msg.write( propInfo[ i ].msgID );
		msg.write( value );
	    }
	    break;
	    
	case MSG_CP_MNY_PREC :
	    /*
	    ** Ignored at lower protocol levels to permit setting
	    ** in property file for all connections.  Translate to
	    ** numeric value.
	    */
	    if ( conn.msg_protocol_level >= MSG_PROTO_3 )
	    {
		msg.write( propInfo[ i ].msgID );
		msg.write( (short)1 );
		
		try { msg.write( Byte.parseByte( value ) ); }
		catch( NumberFormatException ex )
		{ 
		    if ( trace.enabled( 1 ) )
			trace.write( tr_id + ": money prec '" + value + "'" );
		    throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE ); 
		}
	    }
	    break;
	    
	case MSG_CP_DATE_ALIAS :
	    /*
	    ** Ignored at lower protocol levels to permit setting
	    ** in property file for all connections.
	    */
	    if ( value.equalsIgnoreCase( "ingresdate" ) )
		use_ingres_dates = true;
	    else  if ( ! value.equalsIgnoreCase( "ansidate" ) )
	    {
		if ( trace.enabled( 1 ) ) 
		    trace.write( tr_id + ": date alias '" + value + "'" );
		throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
	    }

	    if ( conn.msg_protocol_level >= MSG_PROTO_6 )
	    {
		msg.write( propInfo[ i ].msgID );
		msg.write( value );
	    }

	    date_alias_set = true;
	    break;

	case DRV_CP_SELECT_LOOP :
	    /*
	    ** This is a local connection property
	    ** and is not forwarded to the server.
	    */
	    if ( value.equalsIgnoreCase( "off" ) )
		conn.select_loops = false;
	    else  if ( value.equalsIgnoreCase( "on" ) )
		conn.select_loops = true;
	    else
	    {
		if ( trace.enabled( 1 ) ) 
		    trace.write( tr_id + ": select loop '" + value + "'" );
		throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
	    }
	    break;

	case DRV_CP_CURSOR_MODE :
	    /*
	    ** This is a local connection property
	    ** and is not forwarded to the server.
	    */
	    if ( value.equalsIgnoreCase( "dbms" ) )
		conn.cursor_mode = DRV_CRSR_DBMS;
	    else  if ( value.equalsIgnoreCase( "update" ) )
		conn.cursor_mode = DRV_CRSR_UPDATE;
	    else  if ( value.equalsIgnoreCase( "readonly" ) )
		conn.cursor_mode = DRV_CRSR_READONLY;
	    else
	    {
		if ( trace.enabled( 1 ) ) 
		    trace.write( tr_id + ": cursor mode '" + value + "'" );
		throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
	    }
	    break;

	case DRV_CP_CHAR_ENCODE :
	    /*
	    ** This is a local connection property
	    ** and is not forwarded to the server.
	    **
	    ** Character encoding is processed during
	    ** initial server connection establishment.
	    */
	    break;

	case DRV_CP_SND_ING_DTE :
	    /*
	    ** This is a local connection property
	    ** and is not forwarded to the server.
	    */
	    if ( value.equalsIgnoreCase( "false" ) )
		conn.cnf_flags &= ~conn.CNF_INGDATE;
	    else  if ( value.equalsIgnoreCase( "true" ) ) 
	    {
		conn.cnf_flags |= conn.CNF_INGDATE;
		use_ingres_dates = true;
	    }
	    else
	    {
		if ( trace.enabled( 1 ) ) 
		    trace.write(tr_id + ": send ingres dates '" + value + "'");
		throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
	    }

	    send_ingdate_set = true;
	    break;

	case DRV_CP_SND_INT_BOOL :
	    /*
	    ** This is a local connection property
	    ** and is not forwarded to the server.
	    */
	    if ( value.equalsIgnoreCase( "true" ) ) 
		conn.cnf_flags |= conn.CNF_INTBOOL;
	    else  if ( value.equalsIgnoreCase( "false" ) )
		conn.cnf_flags &= ~conn.CNF_INTBOOL;
	    else
	    {
		if ( trace.enabled( 1 ) ) 
		    trace.write( tr_id + ": send integer booleans '" + 
				 value + "'" );
		throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
	    }
	    break;

	default :
	    msg.write( propInfo[ i ].msgID );
	    msg.write( value );
	    break;
	}

	if ( trace.enabled( 3 ) )  
	    trace.write( "    " + propInfo[ i ].desc + ": '" + value + "'" );
    }

    /*
    ** The date_alias and send_ingres_dates property defaults are
    ** codependent.  If neither is set, the default is ANSI dates.
    ** If one of the two is set, the other should default to a
    ** similar setting.  Nothing to do if both are set.
    **
    ** Note: use_ingres_dates is initialized to false and set to 
    ** true if either of these properties is configured for Ingres 
    ** dates.
    */
    if ( ! date_alias_set  &&  conn.msg_protocol_level >= MSG_PROTO_6 )
    {
	/*
	** The default for the date_alias property depends
	** on the setting for the property send_ingres_dates.
	** If Ingres dates are being used, then default date
	** alias should be "ingresdate".  If ANSI dates are
	** being sent, then "ansidate" should be the default.
	*/
	String value = use_ingres_dates ? "ingresdate" : "ansidate";
	msg.write( MSG_CP_DATE_ALIAS );
	msg.write( value );

	if ( trace.enabled( 3 ) )  
	    trace.write( "    Date Alias (default): '" + value + "'" );
    }

    if ( ! send_ingdate_set )
    {
	/*
	** The default for the send_ingres_dates property depends
	** on the setting for the property date_alias.  If Ingres
	** dates are being used, then default to sending Ingres
	** date values.  If ANSI dates are being used, then ANSI
	** date/time values should be sent.
	*/
	if ( use_ingres_dates )
	    conn.cnf_flags |= conn.CNF_INGDATE;
	else
	    conn.cnf_flags &= ~conn.CNF_INGDATE;

	if ( trace.enabled( 3 ) )  
	    trace.write( "    Send Ingres Dates (default): '" + 
			 use_ingres_dates + "'" );
    }

    return;
} // client_parms


/*
** Name: driver_parms
**
** Description:
**	Send internal DAM-ML connection parameters.
**
** Input:
**	local	Use local user ID.
**	timeout	Connection timeout.
**	ing_xid	Ingres distributed transaction ID.
**	xa_xid	XA transaction ID.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 4-Nov-99 (gordy)
**	    Send timeout connection parameter to server.
**	11-Apr-01 (gordy)
**	    Added timeout parameter to replace DriverManager reference.
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
**	06-Dec-02 (wansh01)
**	    Added support for optional userid for local connection.
**	    set connection parameter MSG_CP_LOGIN_TYPE to MSG_CPV_LOGIN_USER
**	    (an internal value) if no userid specified for local connection. 
**	26-Dec-02 (gordy)
**	    Send initial autocommit state to server.
**	15-Jan-03 (gordy)
**	    Extracted from connect().  Added local user, host name & address.
**	 7-Jul-03 (gordy)
**	    Write all parameters associated with local user connections.
**	16-Jun-04 (gordy)
**	    Added session_mask to password encryption.
*/

private void
driver_parms( boolean local, int timeout, IngXid ing_xid, XaXid xa_xid )
    throws SQLException
{
    String value, userID = null;

    /*
    ** User ID needed for various parameters.
    */
    try { userID = System.getProperty( "user.name" ); }
    catch( Exception ignore ) {}
    if ( userID != null  &&  userID.length() == 0 )  userID = null;
	
    /*
    ** If connection should be made for local user, send the 
    ** appropriate connection parameters. (protocol level 
    ** checked by caller).
    */
    if ( local  &&  userID != null )
    {
	/*
	** A special login type is used to indicate that
	** the local user ID of the client should be used
	** (without requiring a password) to establish
	** the connection.
	*/
	if ( trace.enabled( 3 ) )  trace.write( "    Login Type: 'user'" ); 
	
	msg.write( MSG_CP_LOGIN_TYPE );
	msg.write( (short)1 );
	msg.write( MSG_CPV_LOGIN_USER);
	
	/*
	** If a user ID had been provided, then we would not be
	** doing a local user connection.  Also, a password is 
	** only sent if a user ID is provided.  These parameters
	** are hijacked for this case to send the local user ID
	** with some verification of its authenticity.
	*/
	msg.write( MSG_CP_USERNAME );
	msg.write( userID );
	msg.write( MSG_CP_PASSWORD );
	msg.write( msg.encode( userID, conn.session_mask, userID ) );
    }
    
    /*
    ** JDBC timeout of 0 means no timeout (full blocking).
    ** Only send timeout if non-blocking.
    */
    if ( timeout != 0 )
    {
	if ( trace.enabled( 3 ) )  
	    trace.write( "    Timeout: " + timeout );
	/*
	** Ingres JDBC driver interprets a negative timeout
	** as milli-seconds.  Otherwise, convert seconds to
	** milli-seconds.
	*/
	timeout = (timeout < 0) ? -timeout : timeout * 1000;
	msg.write( MSG_CP_TIMEOUT );
	msg.write( (short)4 );
	msg.write( timeout );
    }

    if ( ing_xid != null  &&  conn.msg_protocol_level >= MSG_PROTO_2 )
    {
	if ( trace.enabled( 3 ) )  
	    trace.write( "    Ingres Transaction ID: " + ing_xid );

	msg.write( MSG_CP_II_XID );
	msg.write( (short)8 );
	msg.write( (int)ing_xid.getID() );
	msg.write( (int)(ing_xid.getID() >> 32) );
	
	/*
	** Autocommit is disabled when connecting to
	** an existing distributed transaction.
	*/
	conn.autoCommit = false;
    }

    if ( xa_xid != null  &&  conn.msg_protocol_level >= MSG_PROTO_2 )
    {
	if ( trace.enabled( 3 ) )  
	    trace.write( "    XA Transaction ID: " + xa_xid );

	msg.write( MSG_CP_XA_FRMT );
	msg.write( (short)4 );
	msg.write( xa_xid.getFormatId() );
	msg.write( MSG_CP_XA_GTID );
	msg.write( xa_xid.getGlobalTransactionId() );
	msg.write( MSG_CP_XA_BQUAL );
	msg.write( xa_xid.getBranchQualifier() );

	/*
	** Autocommit is disabled when connecting to
	** an existing distributed transaction.
	*/
	conn.autoCommit = false;
    }

    if ( conn.msg_protocol_level >= MSG_PROTO_3 )
    {
	/*
	** Force initial autocommit state rather than relying
	** on the server default.
	*/
	if ( trace.enabled( 3 ) )  
	    trace.write( "    Autocommit State: " + 
			 (conn.autoCommit ? "on" : "off") );

	msg.write( MSG_CP_AUTO_STATE );
	msg.write( (short)1 );
	msg.write( (byte)(conn.autoCommit ? MSG_CPV_POOL_ON 
					  : MSG_CPV_POOL_OFF) );

	/*
	** Provide local user ID
	*/
	if ( userID != null )
	{
	    if ( trace.enabled( 3 ) )  
		trace.write( "    Local User ID: " + userID ); 

	    msg.write( MSG_CP_CLIENT_USER );
	    msg.write( userID );
	}

	/*
	** Provide local host name
	*/
	value = msg.getLocalHost();
	if ( value != null  &&  value.length() > 0 )
	{
	    if ( trace.enabled( 3 ) )  
		trace.write( "    Local Host: " + value ); 

	    msg.write( MSG_CP_CLIENT_HOST );
	    msg.write( value );
	}

	/*
	** Provide local host address
	*/
	value = msg.getLocalAddr();
	if ( value != null  &&  value.length() > 0 )
	{
	    if ( trace.enabled( 3 ) )  
		trace.write( "    Local Address: " + value ); 

	    msg.write( MSG_CP_CLIENT_ADDR );
	    msg.write( value );
	}
    }

    return;
} // driver_parms


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
    close();
    super.finalize();
    return;
} // finalize


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
    boolean closed = msg.isClosed();
    if ( trace.enabled() )  trace.log( title + ".isClosed(): " + closed );
    return( closed );
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
    
    if ( msg.isClosed() )	// Nothing to do
    {
	if ( trace.enabled(2) )  
	    trace.write( tr_id + ": connection already closed" );
	return;
    }
    
    msg.lock();
    try
    {
	/*
	** Send disconnect request and read results.
	*/
	if ( trace.enabled(2) )  
	    trace.write( tr_id + ": Closing DBMS connection" ); 

	msg.begin( MSG_DISCONN );
	msg.done( true );
	readResults();

	if ( trace.enabled(2) )  
	    trace.write( tr_id + ": DBMS connection closed" ); 
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".close(): error closing connection" );
	if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace );
	throw ex;
    }
    finally 
    { 
	msg.unlock(); 
	conn.close();
    }

    conn.autoCommit = false;
    return;
} // close


/*
** Name: abort
**
** Description:
**	Abort connection irregardless of current state.
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
**	 2-Feb-05 (gordy)
**	    Created.
*/

void // package access required
abort()
{
    if ( trace.enabled(2) )  trace.write( tr_id + ".abort()" ); 
    msg.abort();
    return;
} // abort()


/*
** Name: createStatement
**
** Description:
**	Creates a JDBC Statement object associated with the connection.
**
**	If ResultSet type is not provided, FORWARD_ONLY will be used.
**	If ResultSet concurrency is not provided, DBMS will be used 
**	(ResultSet concurrency is translated to internal cursor mode).
**	If ResultSet holdability is not provided, the default connection
**	setting for holdability will be used.
**
** Input:
**	type		ResultSet type.
**	concurrency	ResultSet concurrency.
**	holdability	ResultSet holdability
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
**	19-Feb-03 (gordy)
**	    Added ResultSet holdability.  Separated bulk of implementation
**	    into private createStmt() to centralize operations.
*/

public Statement
createStatement()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".createStatement()" );
    return( createStmt( ResultSet.TYPE_FORWARD_ONLY, 
			DRV_CRSR_DBMS, holdability ) );
} // createStatement


public Statement
createStatement( int type, int concurrency )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log(title + ".createStatement(" + type + "," + concurrency + ")");
    
    switch( concurrency )
    {
    case ResultSet.CONCUR_READ_ONLY : concurrency = DRV_CRSR_READONLY;	break;
    case ResultSet.CONCUR_UPDATABLE : concurrency = DRV_CRSR_UPDATE;	break;
    default : warnings = null;	throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    }

    return( createStmt( type, concurrency, holdability ) );
} // createStatement


public Statement
createStatement( int type, int concurrency, int holdability )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".createStatement(" + 
	type + "," + concurrency + "," + holdability + ")" );
    
    switch( concurrency )
    {
    case ResultSet.CONCUR_READ_ONLY : concurrency = DRV_CRSR_READONLY;	break;
    case ResultSet.CONCUR_UPDATABLE : concurrency = DRV_CRSR_UPDATE;	break;
    default : warnings = null;	throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    }

    return( createStmt( type, concurrency, holdability ) );
} // createStatement


/*
** Name: createStmt
**
** Description:
**	Creates a JDBC Statement object associated with the connection.
**
**	ResultSets will always be CLOSE_AT_COMMIT.  Type and concurrency 
**	can affect both cursor mode and the ResultSet type.  Application 
**	values are verified and suitable warnings generated.
**
** Input:
**	type		ResultSet type.
**	concurrency	Cursor concurrency.
**	holdability	ResultSet holdability
**
** Output:
**	None.
**
** Returns:
**	JdbcStmt	A new statement object.
**
** History:
**	19-Feb-03 (gordy)
**	    Centralized statement creation in this method.
**	6-Apr-07 (gordy)
**	    Support scrollable cursors.
*/

private JdbcStmt
createStmt( int type, int concurrency, int holdability )
    throws SQLException
{
    boolean warn = false;
    warnings = null;

    /*
    ** Validate connection state.
    */
    if ( conn.is_dtmc )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": not permitted when DTMC" );
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
    }

    /*
    ** Validate parameter values.  
    */
    switch( type )
    {
    case ResultSet.TYPE_FORWARD_ONLY :		break;

    case ResultSet.TYPE_SCROLL_INSENSITIVE :
    case ResultSet.TYPE_SCROLL_SENSITIVE :
	if ( conn.msg_protocol_level < MSG_PROTO_6 )
	{
	    /*
	    ** Scrollable cursors are not supported - 
	    ** change type and generate warning.
	    */
	    if ( trace.enabled( 3 ) )  trace.write( tr_id + 
		": ResultSet type SCROLL requested, type FORWARD provided" );

	    type = ResultSet.TYPE_FORWARD_ONLY;
	    warn = true;
	}
	break;

    default : throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    }

    switch( concurrency )
    {
    case DRV_CRSR_DBMS :	break;
    case DRV_CRSR_READONLY :	break;
    case DRV_CRSR_UPDATE :	break;
    default : throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    }

    switch( holdability )
    {
    case ResultSet.CLOSE_CURSORS_AT_COMMIT : break;

    case ResultSet.HOLD_CURSORS_OVER_COMMIT :
	/*
	** Holdable cursors are not supported - 
	** change type and generate warning.
	*/
	if ( trace.enabled( 3 ) )  trace.write( tr_id + 
		": ResultSet HOLD requested, type CLOSE provided" );

	holdability = ResultSet.CLOSE_CURSORS_AT_COMMIT;
	warn = true;
	break;

    default : throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    }
    
    JdbcStmt stmt = new JdbcStmt( conn, type, concurrency, holdability );
    if ( warn )  warnings = SqlWarn.get( ERR_GC4016_RS_CHANGED );
    if ( trace.enabled() )  trace.log( title + ".createStatement(): " + stmt );
    return( stmt );
} // createStmt


/*
** Name: prepareStatement
**
** Description:
**	Creates a JDBC PreparedStatement object associated with the
**	connection.
**
**	If ResultSet type is not provided, FORWARD_ONLY will be used.
**	If ResultSet concurrency is not provided, DBMS will be used 
**	(ResultSet concurrency is translated to internal cursor mode).
**	If ResultSet holdability is not provided, the default connection
**	setting for holdability will be used.  Parameters involving
**	auto generated key columns are ignored.
**
** Input:
**	sql		    Query text.
**	type		    ResultSet type.
**	concurrency	    ResultSet concurrency.
**	holdability	    ResultSet holdability
**	autoGeneratedKeys   Return auto gen key values?
**	columnIndexes	    Indexes of auto gen key columns.
**	columnNames	    Names of auto gen key columns.
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
**	19-Feb-03 (gordy)
**	    Added ResultSet holdability.  Separated bulk of implementation
**	    into private createPrep() to centralize operations.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Clean up tracing of array parameter.
*/

public PreparedStatement
prepareStatement( String sql )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".prepareStatement('" + sql + "')" );
    return( createPrep( sql, ResultSet.TYPE_FORWARD_ONLY,
			DRV_CRSR_DBMS, holdability ) );
} // prepareStatement


public PreparedStatement
prepareStatement( String sql, int type, int concurrency )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".prepareStatement('" + 
	sql + "'," + type + "," + concurrency + ")" );
    
    switch( concurrency )
    {
    case ResultSet.CONCUR_READ_ONLY : concurrency = DRV_CRSR_READONLY;	break;
    case ResultSet.CONCUR_UPDATABLE : concurrency = DRV_CRSR_UPDATE;	break;
    default : warnings = null;	throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    }

    return( createPrep( sql, type, concurrency, holdability ) );
} // prepareStatement


public PreparedStatement
prepareStatement( String sql, int type, int concurrency, int holdability )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".prepareStatement('" + 
	sql + "'," + type + "," + concurrency + "," + holdability + ")" );

    switch( concurrency )
    {
    case ResultSet.CONCUR_READ_ONLY : concurrency = DRV_CRSR_READONLY;	break;
    case ResultSet.CONCUR_UPDATABLE : concurrency = DRV_CRSR_UPDATE;	break;
    default : warnings = null;	throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    }

    return( createPrep( sql, type, concurrency, holdability ) );
} // prepareStatement


public PreparedStatement
prepareStatement( String sql, int autoGeneratedKeys )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".prepareStatement('" + 
					sql + "'," + autoGeneratedKeys + ")" );
    switch( autoGeneratedKeys )
    {
    case Statement.RETURN_GENERATED_KEYS :  break;
    case Statement.NO_GENERATED_KEYS :	    break;
    default : throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    }

    return( createPrep( sql, ResultSet.TYPE_FORWARD_ONLY,
			DRV_CRSR_DBMS, holdability ) );
} // prepareStatement


public PreparedStatement
prepareStatement( String sql, int columns[] )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".prepareStatement('" + sql + "'," + 
		   (columns == null ? "null" : "[" + columns.length + "]") + 
		   ")" );

    return( createPrep( sql, ResultSet.TYPE_FORWARD_ONLY,
			DRV_CRSR_DBMS, holdability ) );
} // prepareStatement


public PreparedStatement
prepareStatement( String sql, String columns[] )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".prepareStatement('" + sql + "'," + 
		   (columns == null ? "null" : "[" + columns.length + "]") + 
		   ")" );

    return( createPrep( sql, ResultSet.TYPE_FORWARD_ONLY,
			DRV_CRSR_DBMS, holdability ) );
} // prepareStatement


/*
** Name: createPrep
**
** Description:
**	Creates a JDBC PreparedStatement object associated with the
**	connection.
**
**	ResultSets will always be CLOSE_AT_COMMIT.  Type and concurrency 
**	can affect both cursor mode and the ResultSet type.  Application 
**	values are verified and suitable warnings generated.
**
** Input:
**	sql	    Query text.
**	type	    ResultSet type.
**	concurrency Cursor concurrency.
**	holdability ResultSet holdability
**
** Output:
**	None.
**
** Returns:
**	JdbcPrep    A new prepared statement object.
**
** History:
**	19-Feb-03 (gordy)
**	    Centralized prepared statement creation in this method.
**	6-Apr-07 (gordy)
**	    Support scrollable cursors.
*/

private JdbcPrep
createPrep( String sql, int type, int concurrency, int holdability )
    throws SQLException
{
    boolean warn = false;
    warnings = null;

    /*
    ** Validate connection state.
    */
    if ( conn.is_dtmc )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": not permitted when DTMC" );
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
    }

    /*
    ** Validate parameter values.  
    */
    switch( type )
    {
    case ResultSet.TYPE_FORWARD_ONLY :		break;

    case ResultSet.TYPE_SCROLL_INSENSITIVE :
    case ResultSet.TYPE_SCROLL_SENSITIVE :
	if ( conn.msg_protocol_level < MSG_PROTO_6 )
	{
	    /*
	    ** Scrollable cursors are not supported - 
	    ** change type and generate warning.
	    */
	    if ( trace.enabled( 3 ) )  trace.write( tr_id + 
		": ResultSet type SCROLL requested, type FORWARD provided" );

	    type = ResultSet.TYPE_FORWARD_ONLY;
	    warn = true;
	}
	break;

    default : throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    }

    switch( concurrency )
    {
    case DRV_CRSR_DBMS :	break;
    case DRV_CRSR_READONLY :	break;
    case DRV_CRSR_UPDATE :	break;
    default : throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    }

    switch( holdability )
    {
    case ResultSet.CLOSE_CURSORS_AT_COMMIT : break;

    case ResultSet.HOLD_CURSORS_OVER_COMMIT :
	/*
	** Holdable cursors are not supported - 
	** change type and generate warning.
	*/
	if ( trace.enabled( 3 ) )  trace.write( tr_id + 
		": ResultSet type HOLD requested, type CLOSE provided" );

	holdability = ResultSet.CLOSE_CURSORS_AT_COMMIT;
	warn = true;
	break;

    default : throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    }

    JdbcPrep prep = new JdbcPrep( conn, sql, type, concurrency, holdability );
    if ( warn )  warnings = SqlWarn.get( ERR_GC4016_RS_CHANGED );
    if ( trace.enabled() )  trace.log(title + ".prepareStatement(): " + prep);
    return( prep );
} // createPrep


/*
** Name: prepareCall
**
** Description:
**	Creates a JDBC CallableStatement object associated with the
**	connection.
**
**	If ResultSet type is not provided, FORWARD_ONLY will be used.
**	If ResultSet concurrency is not provided, READ_ONLY will be used 
**	If ResultSet holdability is not provided, the default connection
**	setting for holdability will be used.
**
** Input:
**	sql	    Execute procedure statement.
**	type	    ResultSet type.
**	concurrency ResultSet concurrency.
**	holdability ResultSet holdability
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
**	19-Feb-03 (gordy)
**	    Added ResultSet holdability.  Separated bulk of implementation
**	    into private createCall() to centralize operations.
*/

public CallableStatement 
prepareCall( String sql )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".prepareCall( " + sql + " )" );
    return( createCall( sql, ResultSet.TYPE_FORWARD_ONLY,
			     ResultSet.CONCUR_READ_ONLY, holdability ) );
} // prepareCall


public CallableStatement 
prepareCall( String sql, int type, int concurrency )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".prepareCall('" + 
	sql + "'," + type + "," + concurrency + ")" );
    return( createCall( sql, type, concurrency, holdability ) );
} // prepareCall


public CallableStatement 
prepareCall( String sql, int type, int concurrency, int holdability )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".prepareCall('" + 
	sql + "'," + type + "," + concurrency + "," + holdability + ")" );
    return( createCall( sql, type, concurrency, holdability ) );
} // prepareCall


/*
** Name: createCall
**
** Description:
**	Creates a JDBC CallableStatement object associated with the
**	connection.
**
**	Procedure ResultSets are always FORWARD_ONLY, READONLY and 
**	CLOSE_AT_COMMIT.  Application provided values are verified
**	and suitable warnings generated.
**
** Input:
**	sql	    Execute procedure statement.
**	type	    ResultSet type.
**	concurrency ResultSet concurrency.
**	holdability ResultSet holdability
**
** Output:
**	None.
**
** Returns:
**	JdbcCall    A new callable statement object.
**
** History:
**	19-Feb-03 (gordy)
**	    Centralized callable statement creation in this method.
*/

private JdbcCall
createCall( String sql, int type, int concurrency, int holdability )
    throws SQLException
{
    boolean warn = false;
    warnings = null;

    /*
    ** Validate connection state
    */
    if ( conn.is_dtmc )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": not permitted when DTMC" );
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
    }

    if ( conn.msg_protocol_level < MSG_PROTO_2 )
    {
	if ( trace.enabled( 1 ) ) 
	    trace.write( tr_id + ": procedures require protocol level 2" );
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
    }

    /*
    ** Validate parameter values.  
    */
    switch( type )
    {
    case ResultSet.TYPE_FORWARD_ONLY : break;

    /*
    ** Scrollable results are not supported - 
    ** generate warning for scrolling change.
    */
    case ResultSet.TYPE_SCROLL_INSENSITIVE :
    case ResultSet.TYPE_SCROLL_SENSITIVE : warn = true; break;

    default : throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    }

    switch( concurrency )
    {
    case ResultSet.CONCUR_READ_ONLY : break;

    /*
    ** Updatable results are not supported - 
    ** generate warning for concurrency change.
    */
    case ResultSet.CONCUR_UPDATABLE : warn = true; break;

    default : throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    }

    switch( holdability )
    {
    case ResultSet.CLOSE_CURSORS_AT_COMMIT : break;

    /*
    ** Holdable results are not supported - 
    ** generate warning for holdability change.
    */
    case ResultSet.HOLD_CURSORS_OVER_COMMIT : warn = true; break;

    default : throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    }

    JdbcCall call = new JdbcCall( conn, sql );
    if ( warn )  warnings = SqlWarn.get( ERR_GC4016_RS_CHANGED );
    if ( trace.enabled() )  trace.log( title + ".prepareCall()" + call );
    return( call );
} // createCall


/*
** Name: getHoldability
**
** Description:
**	Returns the current default ResultSet holdability.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Holdability.
**
** History:
**	19-Feb-03 (gordy)
**	    Created.
*/

public int
getHoldability()
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".getHoldability(): " + holdability );
    return( holdability );
} // getHoldability


/*
** Name: setHoldability
**
** Description:
**	Set the default ResultSet holdability:
**
**	Ingres does not support holdable cursors.
**
** Input:
**	holdability	ResultSet holdability
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	19-Feb-03 (gordy)
**	    Created.
*/

public void
setHoldability( int holdability )
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".getHoldability(" + holdability + ")" );
    
    if ( holdability != ResultSet.CLOSE_CURSORS_AT_COMMIT )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": holdable cursors not supported!" );
	throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    }
    
    this.holdability = holdability;
    return;
} // setHoldability


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
**	15-Apr-03 (gordy)
**	    Added connection timezones separate from date formatters.
**	 7-Jul-03 (gordy)
**	    Added Ingres timezone config which affects date/time literals.
**	12-Sep-03 (gordy)
**	    Timezone replaced with GMT indicator.
**	19-Jun-06 (gordy)
**	    I give up... GMT indicator replaced with connection.
*/

public String
nativeSQL( String sql )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".nativeSQL( " + sql + " )" );
    SqlParse parser = new SqlParse( sql, conn );
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
**	31-Oct-02 (gordy)
**	    Single meta-data object per connection.
*/

public DatabaseMetaData
getMetaData()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getMetaData()" );
    if ( dbmd == null )  dbmd = new JdbcDBMD( conn );
    return( dbmd );
} // getMetaData


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
    if ( trace.enabled() )  
	trace.log( title + ".isReadOnly(): " + conn.readOnly );
    return( conn.readOnly );
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
    if ( conn.readOnly == readOnly )  return;
    
    if ( conn.is_dtmc )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": not permitted when DTMC" );
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
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
**	    When accessing EDBC server from JAVA IDEs such as Unify and 
**	    WSAD 'Invalid parameter value" SQLexception is thrown. Since 
**	    the only isolation level supported by the EDBC server is 
**	    SERIALIZABLE, the transaction level is not changed, because 
**	    the default connection transaction level is SERIALIZABLE 
**	    anyway. The SQLexception is not thrown, instead a SQLWarning 
**	    message is generated. 
*/

public synchronized void
setTransactionIsolation( int level )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setTransactionIsolation( " + level + " )" );

    clearWarnings();
    if ( level == this.isolationLevel )  return;

    if ( conn.is_dtmc )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": not permitted when DTMC" );
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
    }

    /* 
    ** If the SQL level is 6.4, only allow TRANSACTION_SERIALIZABLE.
    */
    if ( level != TRANSACTION_SERIALIZABLE  &&  
	 conn.sqlLevel < DBMS_SQL_LEVEL_OI10 )
    {
	/*
	** JDBC spec permits more restrictive transaction 
	** isolation levels when the input transaction level 
	** is not supported by the target DBMS server.
	*/
	if ( trace.enabled( 3 ) ) 
	    trace.write( tr_id + ": Requested isolation level not supported!" );
	setWarning( SqlWarn.get( ERR_GC4019_UNSUPPORTED ) );
	return;
    }
    
    sendTransactionQuery( conn.readOnly, level );
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
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Ensure resources are released even if an exception occurs.
**	    Removed synchronization since called only from synchronized
**	    methods.
*/

private void
sendTransactionQuery( boolean readOnly, int isolationLevel )
    throws SQLException
{
    if ( trace.enabled( 2 ) )  
	trace.write( tr_id + ".sendTransactionQuery( " + 
		   readOnly + " , " + isolationLevel + " )" );

    String sql = "set session read ";
    sql += readOnly ? "only" : "write";
    sql += ", isolation level ";
    Statement stmt = null;

    try
    {
	switch( isolationLevel )
	{
	case TRANSACTION_READ_UNCOMMITTED : sql += "read uncommitted";	break;
	case TRANSACTION_READ_COMMITTED :   sql += "read committed";	break;
	case TRANSACTION_REPEATABLE_READ :  sql += "repeatable read";	break;
	case TRANSACTION_SERIALIZABLE :	    sql += "serializable";	break;
	
	default :
	    throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
	}

	stmt = createStatement();
	stmt.executeUpdate( sql );
	warnings = stmt.getWarnings();
	conn.readOnly = readOnly;
        this.isolationLevel = isolationLevel;
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".sendTransactionQuery(): failed!" );
	if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace );
	throw ex;
    }
    finally
    {
	if ( stmt != null )
	    try { stmt.close(); }  catch( SQLException ignore ) {}
    }

    return;
} // sendTransactionQuery


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
    if ( trace.enabled() )  
	trace.log( title + ".getAutoCommit(): " + conn.autoCommit );
    return( conn.autoCommit );
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
**	14-Apr-03 (gordy)
**	    Clear savepoints.
*/

public void
setAutoCommit( boolean autoCommit )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setAutoCommit( " + autoCommit + " )" );
    warnings = null;
    clearSavepoints( null );
    if ( autoCommit == conn.autoCommit)  return;    // Nothing to do

    if ( conn.is_dtmc )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": not permitted when DTMC" );
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
    }

    msg.lock();
    try
    {
	msg.begin( MSG_XACT );
	msg.write( (short)(autoCommit ? MSG_XACT_AC_ENABLE 
				      : MSG_XACT_AC_DISABLE) );
	msg.done( true );
	readResults();
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".setAutoCommit(): error changing autocommit" );
	if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace );
	throw ex;
    }
    finally 
    { 
	msg.unlock(); 
    }

    conn.autoCommit = autoCommit;
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
**	14-Apr-03 (gordy)
**	    Clear savepoints.  JDBC 3.0 disallows during autocommit.
*/

public void
commit()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".commit()" );
    warnings = null;
    clearSavepoints( null );

    if ( conn.autoCommit )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": not permitted with autocommit enabled" );
	throw SqlExFactory.get( ERR_GC401F_XACT_STATE );
    }

    msg.lock();
    try
    {
	msg.begin( MSG_XACT );
	msg.write( MSG_XACT_COMMIT );
	msg.done( true );
	readResults();
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".commit(): error committing transaction" );
	if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace );
	throw ex;
    }
    finally 
    { 
	msg.unlock(); 
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
**	14-Apr-03 (gordy)
**	    Clear savepoints.  JDBC 3.0 disallows during autocommit.
*/

public void
rollback()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".rollback()" );
    warnings = null;
    clearSavepoints( null );

    if ( conn.autoCommit )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": not permitted with autocommit enabled" );
	throw SqlExFactory.get( ERR_GC401F_XACT_STATE );
    }

    msg.lock();
    try
    {
	msg.begin( MSG_XACT );
	msg.write( MSG_XACT_ROLLBACK );
	msg.done( true );
	readResults();
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".rollback(): error rolling back transaction" );
	if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace );
	throw ex;
    }
    finally 
    { 
	msg.unlock(); 
    }

    return;
} // rollback


/*
** Name: rollback
**
** Description:
**	Rollback to savepoint.
**
** Input:
**	savepoint   Target savepoint.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	14-Feb-03 (gordy)
**	    Created.
**	20-Dec-04 (gordy)
**	    Prepared statements may be dropped when rolling back
**	    to a savepoint, so clear cache to force re-prepare.
*/

public void
rollback( Savepoint savepoint )
    throws SQLException
{
    JdbcSP  sp;

    if ( trace.enabled() )  
	trace.log( title + ".rollback(" + savepoint + ")" );
    warnings = null;

    if ( conn.autoCommit )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": not permitted with autocommit enabled" );
	throw SqlExFactory.get( ERR_GC401F_XACT_STATE );
    }

    for( sp = savepoints; ; sp = sp.getNext() )
	if ( sp == null )
	{
	    if ( trace.enabled( 1 ) )  
		trace.write( tr_id + ": savepoint not found" );
	    throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
	}
	else if ( sp == (JdbcSP)savepoint )
	    break;

    msg.lock();
    try
    {
	msg.begin( MSG_XACT );
	msg.write( MSG_XACT_ROLLBACK );
	msg.write( MSG_XP_SAVEPOINT );
	msg.write( sp.getName() );
	msg.done( true );
	readResults();
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".rollback(): error rolling back savepoint" );
	if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace );
	throw ex;
    }
    finally 
    { 
	msg.unlock(); 
    }

    clearSavepoints( sp );
    conn.clearPrepStmts();
    return;
} // rollback


/*
** Name: setSavepoint
**
** Description:
**	Creates an unnamed savepoint.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Savepoint	The unnamed savepoint.
**
** History:
**	14-Feb-03 (gordy)
**	    Created.
**	20-Mar-03 (gordy)
**	    Check for DBMS support.
*/

public Savepoint
setSavepoint()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".setSavepoint()" );
    warnings = null;

    if ( conn.autoCommit )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": not permitted with autocommit enabled" );
	throw SqlExFactory.get( ERR_GC401F_XACT_STATE );
    }

    if ( conn.msg_protocol_level < MSG_PROTO_3 )
    {
	if ( trace.enabled( 1 ) ) 
	    trace.write( tr_id + ": savepoints not supported @ protocol " + 
				 conn.msg_protocol_level );
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
    }

    String cap = conn.dbCaps.getDbCap( DBMS_DBCAP_SAVEPOINT );
    if ( cap != null  &&  (cap.charAt(0) == 'N'  ||  cap.charAt(0) == 'n') )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": DBMS doesn't support savepoints" );
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
    }

    JdbcSP  sp = JdbcSP.getUnnamedSP();
    createSavepoint( sp );

    if ( trace.enabled() )  trace.log( title + ".setSavepoint: " + sp );
    return( sp );
} // setSavepoint


/*
** Name: setSavepoint
**
** Description:
**	Creates a named savepoint.
**
** Input:
**	name	Savepoint name.
**
** Output:
**	None.
**
** Returns:
**	Savepoint	The named savepoint.
**
** History:
**	14-Feb-03 (gordy)
**	    Created.
**	20-Mar-03 (gordy)
**	    Check for DBMS support.
*/

public Savepoint
setSavepoint( String name )
    throws SQLException
{
    if ( trace.enabled() )  trace.log(title + ".setSavepoint('" + name + "')");
    warnings = null;

    if ( conn.autoCommit )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": not permitted with autocommit enabled" );
	throw SqlExFactory.get( ERR_GC401F_XACT_STATE );
    }

    if ( conn.msg_protocol_level < MSG_PROTO_3 )
    {
	if ( trace.enabled( 1 ) ) 
	    trace.write( tr_id + ": savepoints not supported @ protocol " + 
				 conn.msg_protocol_level );
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
    }

    String cap = conn.dbCaps.getDbCap( DBMS_DBCAP_SAVEPOINT );
    if ( cap != null  &&  (cap.charAt(0) == 'N'  ||  cap.charAt(0) == 'n') )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": DBMS doesn't support savepoints" );
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
    }

    JdbcSP  sp = JdbcSP.getNamedSP( name );
    createSavepoint( sp );

    if ( trace.enabled() )  trace.log( title + ".setSavepoint: " + sp );
    return( sp );
} // setSavepoint


/*
** Name: releaseSavepoint
**
** Description:
**	Remove savepoint from transaction.  Ingres does not allow
**	savepoints to be deleted, so we just remove the savepoint
**	from our list, effectively making the savepoint unusable.
**
** Input:
**	savepoint   Target savepoint.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	14-Feb-03 (gordy)
**	    Created.
*/

public void
releaseSavepoint( Savepoint savepoint )
    throws SQLException
{
    JdbcSP  sp, prev = null;

    if ( trace.enabled() )  
	trace.log( title + ".releaseSavepoint(" + savepoint + ")" );

    for( sp = savepoints; ; prev = sp, sp = sp.getNext() )
	if ( sp == null )
	{
	    if ( trace.enabled( 1 ) )  
		trace.write( tr_id + ": savepoint not found" );
	    throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
	}
	else if ( sp == (JdbcSP)savepoint )
	{
	    if ( prev == null )
		savepoints = sp.setNext( null );
	    else
		prev.setNext( sp.setNext( null ) );
	    break;
	}

    return;
} // releaseSavepoint


/*
** Name: createSavepoint
**
** Description:
**	Send request to server to create a savepoint.
**
** Input:
**	sp	Savepoint.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	14-Feb-03 (gordy)
**	    Created.
*/

private void
createSavepoint( JdbcSP sp )
    throws SQLException
{
    msg.lock();
    try
    {
	msg.begin( MSG_XACT );
	msg.write( MSG_XACT_SAVEPOINT );
	msg.write( MSG_XP_SAVEPOINT );
	msg.write( sp.getName() );
	msg.done( true );
	readResults();
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".setSavepoint: error creating savepoint" );
	if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace );
	throw ex;
    }
    finally 
    { 
	msg.unlock(); 
    }

    sp.setNext( savepoints );
    savepoints = sp;
    return;
} // createSavepoint


/*
** Name: clearSavepoints
**
** Description:
**	Remove savepoints from savepoint chain upto (but excluding)
**	target savepoint.  Use NULL to remove all savepoints.  
**
**	Warning: All savepoints will be removed if target is not found.
**
** Input:
**	sp	Target savepoint, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	14-Feb-03 (gordy)
**	    Created.
*/

private void
clearSavepoints( JdbcSP sp )
{
    while( savepoints != null  &&  savepoints != sp )
	savepoints = savepoints.setNext( null );
    return;
} // clearSavepoints


/*
** Name: getTransactionServer
**
** Description:
**	Returns the server target address of the current session
**	when the server supports the abortTransaction() method.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	    Server target address or NULL if not supported.
**
** History:
**	10-May-05 (gordy)
**	    Created.
*/

String // package access required
getTransactionServer()
{
    String server = null;
    
    /*
    ** While the info we are retrieving is protocol independent,
    ** the subsequent actions related to this info do require a
    ** specific protocol.  This is a little kludgy, but since the
    ** protocol level is not currently exposed we make the check 
    ** ourselves.
    */
    if ( conn.msg_protocol_level >= MSG_PROTO_4 )
	try { server = conn.dbInfo.getDbmsInfo( DBMS_DBINFO_SERVER ); }
	catch( SQLException ignore ) {}
    
    return( server );
} // getTransactionServer()


/*
** Name: startTransaction
**
** Description:
**	Start a distributed transaction.  No transaction, autocommit
**	or normal, may be active.
**
** Input:
**	xid	Ingres XID.
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
**	31-Oct-02 (gordy)
**	    Separated Ingres and XA transaction ID classes.
*/

public void
startTransaction( IngXid xid )
    throws SQLException
{
    if ( trace.enabled( 2 ) )  
	trace.write( tr_id + ".startTransaction(" + xid + ")" );

    warnings = null;

    if ( conn.is_dtmc )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ".startTransaction: not permitted when DTMC" );
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
    }

    if ( conn.msg_protocol_level < MSG_PROTO_2 )
    {
	if ( trace.enabled( 1 ) ) 
	    trace.write( tr_id + ".startTransaction: protocol = " + 
			 conn.msg_protocol_level );
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
    }

    msg.lock();
    try
    {
	msg.begin( MSG_XACT );
	msg.write( MSG_XACT_BEGIN );
	msg.write( MSG_XP_II_XID );
	msg.write( (short)8 );
	msg.write( (int)xid.getID() );
	msg.write( (int)(xid.getID() >> 32) );
	msg.done( true );
	readResults();
    }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )  
	{
	    trace.write( tr_id + ".startTransaction: error starting xact" );
	    SqlExFactory.trace(ex, trace );
	}
	throw ex;
    }
    finally 
    { 
	msg.unlock(); 
    }

    return;
} // startTransaction


/*
** Name: startTransaction
**
** Description:
**	Start a distributed transaction.  No transaction, autocommit
**	or normal, may be active.
**
** Input:
**	xid	XA XID.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	20-Jul-06 (gordy)
**	    Created.
*/

public void
startTransaction( XaXid xid )
    throws SQLException
{
    startTransaction( xid, 0 );
    return;
} // startTransaction


/*
** Name: startTransaction
**
** Description:
**	Start a distributed transaction.  No transaction, autocommit
**	or normal, may be active.
**
** Input:
**	xid	XA XID.
**	flags	XA Flags.
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
**	31-Oct-02 (gordy)
**	    Separated Ingres and XA transaction ID classes.
**	20-Jul-06 (gordy)
**	    Added flags parameter.
*/

public void
startTransaction( XaXid xid, int flags )
    throws SQLException, XaEx
{
    if ( trace.enabled( 2 ) )  
	trace.write( tr_id + ".startTransaction( " + xid + ", 0x" +
		     Integer.toHexString( flags ) + " )" );

    warnings = null;

    if ( conn.is_dtmc )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ".startTransaction: not permitted when DTMC" );
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
    }

    if ( conn.msg_protocol_level < MSG_PROTO_2  ||
         (flags != 0  &&  conn.msg_protocol_level < MSG_PROTO_5) )
    {
	if ( trace.enabled( 1 ) ) 
	    trace.write( tr_id + ".startTransaction: protocol = " + 
			 conn.msg_protocol_level );
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
    }

    msg.lock();
    try
    {
	msg.begin( MSG_XACT );
	msg.write( MSG_XACT_BEGIN );
	msg.write( MSG_XP_XA_FRMT );
	msg.write( (short)4 );
	msg.write( xid.getFormatId() );
	msg.write( MSG_XP_XA_GTID );
	msg.write( xid.getGlobalTransactionId() );
	msg.write( MSG_XP_XA_BQUAL );
	msg.write( xid.getBranchQualifier() );

	if ( flags != 0 )
	{
	    msg.write( MSG_XP_XA_FLAGS );
	    msg.write( (short)4 );
	    msg.write( flags );
	}

	msg.done( true );
	readResults();
    }
    catch( XaEx xaEx )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( tr_id + 
			 ".startTransaction: XA error starting xact - " + 
			 xaEx.getErrorCode() );
	throw xaEx;
    }
    catch( SQLException sqlEx )
    {
	if ( trace.enabled( 1 ) )  
	{
	    trace.write( tr_id + ".startTransaction: error starting xact" );
	    SqlExFactory.trace(sqlEx, trace );
	}
	throw sqlEx;
    }
    finally 
    { 
	msg.unlock(); 
    }

    return;
} // startTransaction


/*
** Name: endTransaction
**
** Description:
**	End XA transaction association.
**
** Input:
**	xid	XA XID.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	20-Jul-06 (gordy)
**	    Created.
*/

public void
endTransaction( XaXid xid )
    throws SQLException, XaEx
{
    endTransaction( xid, 0 );
    return;
} // endTransaction


/*
** Name: endTransaction
**
** Description:
**	End XA transaction association.
**
** Input:
**	xid	XA XID.
**	flags	XA Flags.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	20-Jul-06 (gordy)
**	    Created.
*/

public void
endTransaction( XaXid xid, int flags )
    throws SQLException, XaEx
{
    if ( trace.enabled( 2 ) )  
	trace.write( tr_id + ".endTransaction( " + xid + ", 0x" + 
		     Integer.toHexString( flags ) + " )" );

    warnings = null;

    if ( conn.is_dtmc )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ".endTransaction: not permitted when DTMC" );
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
    }

    if ( conn.msg_protocol_level < MSG_PROTO_5 )
    {
	if ( trace.enabled( 1 ) ) 
	    trace.write( tr_id + ".endTransaction: protocol = " + 
			 conn.msg_protocol_level );
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
    }

    msg.lock();
    try
    {
	msg.begin( MSG_XACT );
	msg.write( MSG_XACT_END );
	msg.write( MSG_XP_XA_FRMT );
	msg.write( (short)4 );
	msg.write( xid.getFormatId() );
	msg.write( MSG_XP_XA_GTID );
	msg.write( xid.getGlobalTransactionId() );
	msg.write( MSG_XP_XA_BQUAL );
	msg.write( xid.getBranchQualifier() );

	if ( flags != 0 )
	{
	    msg.write( MSG_XP_XA_FLAGS );
	    msg.write( (short)4 );
	    msg.write( flags );
	}

	msg.done( true );
	readResults();
    }
    catch( XaEx xaEx )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( tr_id + ".endTransaction: XA error ending xact - " +
			 xaEx.getErrorCode() );
	throw xaEx;
    }
    catch( SQLException sqlEx )
    {
	if ( trace.enabled( 1 ) )  
	{
	    trace.write( tr_id + ".endTransaction: error ending xact" );
	    SqlExFactory.trace(sqlEx, trace );
	}
	throw sqlEx;
    }
    finally 
    { 
	msg.unlock(); 
    }

    return;
} // endTransaction


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
    if ( trace.enabled( 2 ) )  trace.write( tr_id + ".prepareTransaction()" );
    warnings = null;

    if ( conn.is_dtmc )
    {
	if ( trace.enabled( 1 ) )
	    trace.write(tr_id + ".prepareTransaction: not permitted when DTMC");
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
    }

    if ( conn.msg_protocol_level < MSG_PROTO_2 )
    {
	if ( trace.enabled( 1 ) )  trace.write( tr_id + 
	    ".prepareTransaction: protocol = " + conn.msg_protocol_level );
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
    }

    msg.lock();
    try
    {
	msg.begin( MSG_XACT );
	msg.write( MSG_XACT_PREPARE );
	msg.done( true );
	readResults();
    }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )  
	{
	    trace.write(tr_id + ".prepareTransaction: error preparing xact");
	    SqlExFactory.trace(ex, trace );
	}
	throw ex;
    }
    finally 
    { 
	msg.unlock(); 
    }

    return;
} // prepareTransaction


/*
** Name: prepareTransaction
**
** Description:
**	Prepare and XA transaction.
**
** Input:
**	xid	XA XID.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	20-Jul-06 (gordy)
**	    Created.
*/

public void
prepareTransaction( XaXid xid )
    throws SQLException, XaEx
{
    if ( trace.enabled( 2 ) )  
	trace.write( tr_id + ".prepareTransaction(" + xid + ")" );

    warnings = null;

    if ( conn.msg_protocol_level < MSG_PROTO_5 )
    {
	if ( trace.enabled( 1 ) ) 
	    trace.write( tr_id + ".prepareTransaction: protocol = " + 
			 conn.msg_protocol_level );
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
    }

    msg.lock();
    try
    {
	msg.begin( MSG_XACT );
	msg.write( MSG_XACT_PREPARE );
	msg.write( MSG_XP_XA_FRMT );
	msg.write( (short)4 );
	msg.write( xid.getFormatId() );
	msg.write( MSG_XP_XA_GTID );
	msg.write( xid.getGlobalTransactionId() );
	msg.write( MSG_XP_XA_BQUAL );
	msg.write( xid.getBranchQualifier() );
	msg.done( true );
	readResults();
    }
    catch( XaEx xaEx )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( tr_id + 
			 ".prepareTransaction: XA error preparing xact - " + 
			 xaEx.getErrorCode() );
	throw xaEx;
    }
    catch( SQLException sqlEx )
    {
	if ( trace.enabled( 1 ) )  
	{
	    trace.write( tr_id + ".prepareTransaction: error preparing xact" );
	    SqlExFactory.trace(sqlEx, trace );
	}
	throw sqlEx;
    }
    finally 
    { 
	msg.unlock(); 
    }

    return;
} // prepareTransaction


/*
** Name: commit
**
** Description:
**	Commit XA transaction.
**
** Input:
**	xid	XA XID.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	20-Jul-06 (gordy)
**	    Created.
*/

public void
commit( XaXid xid )
    throws SQLException, XaEx
{
    commit( xid, 0 );
    return;
} // commit


/*
** Name: commit
**
** Description:
**	Commit XA transaction.
**
** Input:
**	xid	XA XID.
**	flags	XA Flags.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	20-Jul-06 (gordy)
**	    Created.
*/

public void
commit( XaXid xid, int flags )
    throws SQLException, XaEx
{
    if ( trace.enabled( 2 ) )  
	trace.write( tr_id + ".commit( " + xid + ", 0x" + 
		     Integer.toHexString( flags ) + " )" );

    warnings = null;

    if ( conn.msg_protocol_level < MSG_PROTO_5 )
    {
	if ( trace.enabled( 1 ) ) trace.write( tr_id + 
	    ".commit: protocol = " + conn.msg_protocol_level );
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
    }

    msg.lock();
    try
    {
	msg.begin( MSG_XACT );
	msg.write( MSG_XACT_COMMIT );
	msg.write( MSG_XP_XA_FRMT );
	msg.write( (short)4 );
	msg.write( xid.getFormatId() );
	msg.write( MSG_XP_XA_GTID );
	msg.write( xid.getGlobalTransactionId() );
	msg.write( MSG_XP_XA_BQUAL );
	msg.write( xid.getBranchQualifier() );

	if ( flags != 0 )
	{
	    msg.write( MSG_XP_XA_FLAGS );
	    msg.write( (short)4 );
	    msg.write( flags );
	}

	msg.done( true );
	readResults();
    }
    catch( XaEx xaEx )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( tr_id + ".commit: XA error committing transaction - " +
			 xaEx.getErrorCode() );
	throw xaEx;
    }
    catch( SQLException sqlEx )
    {
	if ( trace.enabled( 1 ) )  
	{
	    trace.write( tr_id + ".commit: error committing transaction" );
	    SqlExFactory.trace(sqlEx, trace );
	}
	throw sqlEx;
    }
    finally 
    { 
	msg.unlock(); 
    }

    return;
} // commit


/*
** Name: rollback
**
** Description:
**	Rollback XA transaction.
**
** Input:
**	xid	XA XID.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	20-Jul-06 (gordy)
**	    Created.
*/

public void
rollback( XaXid xid )
    throws SQLException, XaEx
{
    if ( trace.enabled( 2 ) )  trace.write( tr_id + ".rollback(" + xid + ")" );
    warnings = null;

    if ( conn.msg_protocol_level < MSG_PROTO_5 )
    {
	if ( trace.enabled( 1 ) ) trace.write( tr_id + 
	    ".rollback: protocol = " + conn.msg_protocol_level );
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
    }

    msg.lock();
    try
    {
	msg.begin( MSG_XACT );
	msg.write( MSG_XACT_ROLLBACK );
	msg.write( MSG_XP_XA_FRMT );
	msg.write( (short)4 );
	msg.write( xid.getFormatId() );
	msg.write( MSG_XP_XA_GTID );
	msg.write( xid.getGlobalTransactionId() );
	msg.write( MSG_XP_XA_BQUAL );
	msg.write( xid.getBranchQualifier() );
	msg.done( true );
	readResults();
    }
    catch( XaEx xaEx )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( tr_id + 
			 ".rollback: XA error rolling back transaction - " + 
			 xaEx.getErrorCode() );
	throw xaEx;
    }
    catch( SQLException sqlEx )
    {
	if ( trace.enabled( 1 ) )  
	{
	    trace.write( tr_id + ".rollback: error rolling back transaction" );
	    SqlExFactory.trace(sqlEx, trace );
	}
	throw sqlEx;
    }
    finally 
    { 
	msg.unlock(); 
    }

    return;
} // rollback


/*
** Name: abortTransaction
**
** Description:
**	Abort a distributed transaction which is not active on the 
**	current session.  Requires a DTM connection to the server
**	identified by getTransactionServer() of the session that
**	is associated with the transaction.
**
** Input:
**	xid	XA XID.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	10-May-05 (gordy)
**	    Created.
*/

void // package access required
abortTransaction( XaXid xid )
    throws SQLException
{
    warnings = null;

    if ( trace.enabled() )  
	trace.log( title + ".abortTransaction( '" + xid + "' )" );

    if ( ! conn.is_dtmc )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": DTM connection required" );
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
    }

    if ( conn.msg_protocol_level < MSG_PROTO_4 )
    {
	if ( trace.enabled( 1 ) ) 
	    trace.write( tr_id + ": protocol = " + conn.msg_protocol_level );
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
    }

    msg.lock();
    try
    {
	msg.begin( MSG_XACT );
	msg.write( MSG_XACT_ABORT );
	msg.write( MSG_XP_XA_FRMT );
	msg.write( (short)4 );
	msg.write( xid.getFormatId() );
	msg.write( MSG_XP_XA_GTID );
	msg.write( xid.getGlobalTransactionId() );
	msg.write( MSG_XP_XA_BQUAL );
	msg.write( xid.getBranchQualifier() );
	msg.done( true );
	readResults();
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".abortTransaction(): error aborting xact" );
	if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace );
	throw ex;
    }
    finally 
    { 
	msg.unlock(); 
    }

    return;
} // abortTransaction


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
**	XaXid[]	Transaction IDs.
**
** History:
**	18-Apr-01 (gordy)
**	    Created.
*/

XaXid[] // package access required
getPreparedTransactionIDs( String db )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getPreparedTransactionIDs('" + db +"')" );
    warnings = null;

    if ( conn.msg_protocol_level >= MSG_PROTO_2 )
    {
	/*
	** Request the info from the JDBC Server.
	*/
	if ( dbXids == null )  dbXids = new DbXids( conn );
	return( dbXids.readXids( db ) );
    }

    return( new XaXid[ 0 ] );
} // getPreparedTransactionIDs

/*
** Name: CreateStruct
**
** Description:
**	Factory method for creating Struct objects. The Struct datatype is
**	not supported.
**
** Input:
**	typeName	The SQL type name of the SQL structured type 
**			that this java.sql.Struct object maps to.
**	attributes	The attributes that populate the Struct object.
**
** Output:
**	None.
**
** Returns:
**	Throws SqlFeatureNotSupportedException.
**
** History:
**	 14-Oct-08 (rajus01)
**	    Created.
*/
public Struct 
createStruct(String typeName, Object[] attributes)
throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".createStruct(..)" );
    throw SqlExFactory.get(ERR_GC4019_UNSUPPORTED);
}

/*
** Name: CreateArrayOf
**
** Description:
**	Factory method for creating Array objects. The Array datatype is
**	not supported.
**
** Input:
**	typeName	The SQL type name of the SQL structured type 
**			that this java.sql.Array object maps to.
**	elements	The elements that populate the Array object.
**
** Output:
**	None.
**
** Returns:
**	Throws SqlFeatureNotSupportedException.
**
** History:
**	 05-May-09 (rajus01) SIR 121238
**	    Implemented.
*/
public Array 
createArrayOf(String typeName, Object[] elements)
throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".createArrayOf(..)" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
}

/*
** Name: getClientInfo
**
** Description:
**      Returns a list containing the name and current value of each client
**      info property supported by the driver. The value of a client info
**      property may be null if the property has not been set and does not
**      have a default value.
**
** Input:
**      None.
**
** Output:
**      None.
**
** Returns:
**      An empty property list with no default values.
**
** History:
**      03-Feb-09 (rajus01) SIR 121238
**          Implemented.
*/
public Properties 
getClientInfo()
throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getClientInfo()" );
    return ( new Properties() );
}

/*
** Name: getClientInfo
**
** Description:
**      Returns null as the client info property not supported by the driver.
**
** Input:
**      name  The name of the client info property to retrieve.
**
** Output:
**      None.
**
** Returns:
**      The value of the client info property specified.
**
** History:
**      03-Feb-09 (rajus01) SIR 121238
**          Implemented.
*/
public String 
getClientInfo(String name)
throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getClientInfo(...)" );
    return null;
}

/*
** Name: setClientInfo
**
** Description:
**      Sets the value of the connection's client info properties.
**
** Input:
**      prop  The name of the client info property to retrieve.
**
** Output:
**      None.
**
** Returns:
**      None.
**
** History:
**      03-Feb-09 (rajus01) SIR 121238
**          Implemented.
*/
public void 
setClientInfo(Properties prop)
throws SQLClientInfoException
{
    if ( trace.enabled() )  
	trace.log( title + ".setClientInfo(...)" );
    if (prop == null || prop.size() == 0)
	return;
    Map<String, ClientInfoStatus> failMap = 
			new HashMap<String, ClientInfoStatus>();
    Iterator<String> itr = prop.stringPropertyNames().iterator();
    while (itr.hasNext())
       failMap.put(itr.next(), ClientInfoStatus.REASON_UNKNOWN_PROPERTY);
    throw new SQLClientInfoException(
                SqlExFactory.get(ERR_GC4019_UNSUPPORTED).getMessage(), failMap);
}

/*
** Name: setClientInfo
**
** Description:
**      Sets the value of the client info property specified by name to
**      the value specified by value.
**
** Input:
**      name    The name of the client info property to set.
**      value   The value to set the client info property to.
**
** Output:
**      None.
**
** Returns:
**      None.
**
** History:
**      03-Feb-09 (rajus01) SIR 121238
**          Implemented.
*/
public void 
setClientInfo(String name, String value)
throws SQLClientInfoException
{
    if ( trace.enabled() )  
	trace.log( title + ".setClientInfo(...)" );
    Map<String, ClientInfoStatus> failMap = 
			new HashMap<String, ClientInfoStatus>();
    failMap.put(name, ClientInfoStatus.REASON_UNKNOWN_PROPERTY);
    throw new SQLClientInfoException(
                SqlExFactory.get(ERR_GC4019_UNSUPPORTED).getMessage(), failMap);
}

/*
** Name: createSQLXML 
**
** Description:
**	Constructs an object that implements the SQLXML interface. The SQLXML
**	data type is not supported.
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Throws SqlFeatureNotSupportedException.
**
** History:
**	 05-May-09 (rajus01) SIR 121238
**	    Implemented.
*/
public SQLXML 
createSQLXML()
throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".createSQLXML()" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
}

/*
** Name: createNClob
**
** Description:
**      Constructs BufferedNlob object. 
**
** Input:
**      None.
**
** Output:
**      None.
**
** Returns:
**	NClob.
**
** History:
**	 05-May-09 (rajus01) SIR 121238
**	    Implemented.
*/
public NClob 
createNClob()
throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".createNClob()" );
    return( new JdbcNlob( conn.cnf_lob_segSize, trace ) );
}

/*
** Name: createBlob
**
** Description:
**      Constructs BufferedBlob object. 
**
** Input:
**      None.
**
** Output:
**      None.
**
** Returns:
**	Blob.
**
** History:
**	 05-May-09 (rajus01) SIR 121238
**	    Implemented.
*/
public Blob 
createBlob()
throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".createBlob()" );
    return( new JdbcBlob( conn.cnf_lob_segSize, trace ) );
}

/*
** Name: createClob
**
** Description:
**      Constructs BufferedClob object. 
**
** Input:
**      None.
**
** Output:
**      None.
**
** Returns:
**	Clob.
**
** History:
**	 05-May-09 (rajus01) SIR 121238
**	    Implemented.
*/
public Clob 
createClob()
throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".createClob()" );
    return( new JdbcClob( conn.cnf_lob_segSize, trace ) );
}

/*
** Name: isValid
**
** Description:
**	Determine if the connection is still valid.
**
** Input:
**	timeout	// The time in seconds.
**
** Output:
**	None.
**
** Returns:
**	boolean	// True if connection closed, false otherwise.
**
** History:
**	 5-May-09 (rajus01) 121238
**	    Implemented.
**	22-Jun-09 (rajus01)
**	    Return true in case of no SQLException.
*/
public boolean 
isValid(int timeout)
throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".isValid( '" + timeout + "' )" );

    if( isClosed() ) return false;

    String sql = "select user_name from iidbconstants";
    Statement stmt = null;
    ResultSet rs = null;

    try
    {
	stmt = createStatement();
	stmt.setQueryTimeout( timeout );
	rs = stmt.executeQuery(sql);
	while( rs.next() ) rs.getString(1);
    }
    catch( SQLException ignore ) { return false; }
    finally
    {
	if ( rs != null )
	    try { rs.close(); }  catch( SQLException ignore ) {}
	if ( stmt != null )
	    try { stmt.close(); }  catch( SQLException ignore ) {}
    }

    return true;
}

/*
** Name: isWrapperFor
**
** Description:
**      Returns true if this either implements the interface argument or is
**      directly or indirectly a wrapper for an object that does. Returns
**      false otherwise.
**
** Input:
**      iface   Class defining an interface
**
** Output:
**      None.
**
** Returns:
**      true or false.
**
** History:
**      23-Jun-09 (rajus01)
**         Implemented.
*/
public boolean
isWrapperFor(Class<?> iface) throws SQLException
{
    if ( trace.enabled() )
       trace.log(title + ".isWrapperFor(" + iface + ")");
    /*
    ** This approach seems to work for classes that actually don't
    ** wrap anything.
    */
    if( iface != null )
	return( iface.isInstance( this ) );

    throw SqlExFactory.get(ERR_GC4010_PARAM_VALUE);

}

/*
** Name: unwrap
**
** Description:
**      Returns an object that implements the given interface to allow
**      access to non-standard methods, or standard methods not exposed by the
**      proxy. If the receiver implements the interface then the result is the
**      receiver or a proxy for the receiver. If the receiver is a wrapper and
**      the wrapped object implements the interface then the result is the
**      wrapped object or a proxy for the wrapped object. Otherwise return the
**      the result of calling unwrap recursively on the wrapped object or
**      a proxy for that result. If the receiver is not a wrapper and does not
**      implement the interface, then an SQLException is thrown.
**
** Input:
**      iface   A Class defining an interface that the result must implement.
**
** Output:
**      None.
**
** Returns:
**      An object that implements the interface.
**
** History:
**      23-Jun-09 (rajus01)
**         Implemented.
*/
public <T> T
unwrap(Class<T> iface) throws SQLException
{
    if ( trace.enabled() )
       trace.log(title + ".unwrap(" + iface + ")");
    /*
    ** This approach seems to work for classes that actually don't
    ** wrap anything.
    */
    if( iface != null )
    {
	if( ! iface.isInstance( this ) )
	    throw SqlExFactory.get(ERR_GC4023_NO_OBJECT); 
	return( iface.cast(this) );
    }

    throw SqlExFactory.get(ERR_GC4010_PARAM_VALUE);
}

/*
** Name: DbXids
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
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
*/

private static class
DbXids
    extends DrvObj
{

    private static JdbcRSMD	rsmd = null;
    private Vector		xids = new Vector();


/*
** Name: DbXids
**
** Description:
**	Class constructor.
**
** Input:
**	conn	Database connection.
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
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
*/

public
DbXids( DrvConn conn )
{
    super( conn );
    title = trace.getTraceName() + "-DbXids[" + msg.connID() + "]";
    tr_id = "DbXid[" + msg.connID() + "]";
    return;
}


/*
** Name: readXids
**
** Description:
**	Query DBMS for transaction IDs.
**
** Input:
**	db	Database name.
**
** Output:
**	None.
**
** Returns:
**	XaXid[]	Transaction IDs.
**
** History:
**	18-Apr-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
*/

public synchronized XaXid[]
readXids( String db )
    throws SQLException
{
    xids.clear();
    msg.lock();
    try
    {
	msg.begin( MSG_REQUEST );
	msg.write( MSG_REQ_2PC_XIDS );
	msg.write( MSG_RQP_DB_NAME );
	msg.write( db );
	msg.done( true );
	readResults();
    }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( tr_id + ": error retrieving XID list" );
	throw ex;
    }
    finally
    {
	msg.unlock();
    }

    /*
    ** XIDs have been collected in xids.
    ** Allocate and populate the XID array.
    */
    XaXid xid[] = new XaXid[ xids.size() ];

    for( int i = 0; i < xid.length; i++ )
	xid[ i ] = (XaXid)xids.get( i );

    xids.clear();
    return( xid );
} // readXids


/*
** Name: readDesc
**
** Description:
**	Read a data descriptor message.  Overrides default method in 
**	DrvObj.  It handles the reading of descriptor messages for 
**	the method readXids (above).
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	JdbcRSMD    Data descriptor.
**
** History:
**	18-Apr-01 (gordy)
**	    Created.
*/

protected JdbcRSMD
readDesc()
    throws SQLException
{
    if ( rsmd == null )
	rsmd = JdbcRSMD.load( conn );
    else
	rsmd.reload( conn );

    // TODO: validate descriptor
    return( rsmd );
} // readDesc


/*
** Name: readData
**
** Description:
**	Read a data message.  This method overrides the readData method 
**	of DrvObj and is called by the readResults method.  It handles 
**	the reading of data messages for the readXids() method (above).  
**	The data should be in a pre-defined format (rows containing 3 
**	non-null columns of type int, byte[], byte[]).
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
    throws SQLException
{
    while( msg.moreData() )
    {
	// TODO: validate data
	msg.readByte();			// NULL byte (never null)
	int fid = msg.readInt();	// Format ID
	msg.readByte();			// NULL byte (never null)
	byte gtid[] = msg.readBytes();	// Global Transaction ID
	msg.readByte();			// NULL byte (never null)
	byte bqual[] = msg.readBytes();	// Branch Qualifier
	xids.add( new XaXid( fid, gtid, bqual ) );
    }

    return( false );
} // readData


} // class DbXids

} // class JdbcConn
