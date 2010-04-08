/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbcx;

/*
** Name: EdbcCPVirtConn.java
**
** Description:
**	Implements the EDBC JDBC extension class EdbcCPVirtConn.
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
**	12-Apr-01 (gordy)
**	    Exceptions are now self tracing.
*/

import	java.util.Map;
import	java.sql.Connection;
import	java.sql.Statement;
import	java.sql.PreparedStatement;
import	java.sql.CallableStatement;
import	java.sql.DatabaseMetaData;
import	java.sql.SQLException;
import	java.sql.SQLWarning;
import	javax.sql.PooledConnection;
import	javax.sql.ConnectionEvent;
import	javax.sql.ConnectionEventListener;
import	ca.edbc.jdbc.EdbcTrace;
import	ca.edbc.util.EdbcEx;


/*
** Name: EdbcCPVirtConn
**
** Description:
**	EDBC JDBC extension class which implements the JDBC 
**	Connection interface as a cover for another JDBC 
**	connection.
**
**	This class wraps the interface methods of the 
**	associated connection to provide connection close
**	request and error event notification to a single
**	event listener.
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
**  Private Data
**
**	trace			DataSource tracing.
**	title			Class tracing title.
**	tr_id			Class tracing ID.
**	inst_id			Instance ID.
**	conn			JDBC connection.
**	pool			Pooled connection owner.
**	listener		Connection event listener.
**	inst_count		Class instance counter.
**
**  Private Methods:
**
**	checkConn		Check for active connection.
**	closeEvent		Raise a connection closed event.
**	errorEvent		Raise a connection error event.
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
*/

class
EdbcCPVirtConn
    implements	Connection, EdbcDSErr
{

    /*
    ** Tracing
    */
    protected EdbcTrace			trace = null;
    protected String			title = null;
    protected String			tr_id = null;
    protected int			inst_id = 0;

    /*
    ** Connection info
    */
    private Connection			conn = null;
    private PooledConnection		pool = null;
    private ConnectionEventListener	listener = null;

    /*
    ** Instance counter to generate instance IDs.
    */
    private static int			inst_count = 0;


/*
** Name: EdbcCPVirtConn
**
** Description:
**	Class constructor.
**
** Input:
**	conn	    JDBC connection.
**	pool	    Pooled connection owner.
**	listener    Connection event listener.
**
** Output:
**	None.
**
**
** Returns:
**	None.
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
*/

public
EdbcCPVirtConn
( 
    Connection		    conn,
    PooledConnection	    pool,
    ConnectionEventListener listener,
    EdbcTrace		    trace
)
{
    this.conn = conn;
    this.pool = pool;
    this.listener = listener;
    this.trace = trace;
    inst_id = inst_count++;
    title = "EDBC-PoolVirtConn[" + inst_id + "]";
    tr_id = "CPVConn[" + inst_id + "]";
} // EdbcCPVirtConn


/*
** Name: toString
**
** Description:
**	Returns a string identfier for this instance.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	    Instance identifier.
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
*/

public String
toString()
{
    return( title );
} // toString


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
**	29-Jan-01 (gordy)
**	    Created.
*/

public boolean
isClosed()
    throws SQLException
{
    return( (conn != null) ? conn.isClosed() : true );
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
**	29-Jan-01 (gordy)
**	    Created.
*/

public void
close()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".close()" );
    if ( conn != null )  closeEvent();
    return;
} // close


/*
** Name: getWarnings
**
** Description:
**	Retrieve the connection warning/message list.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	SQLWarning  SQL warning/message list, may be NULL.
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
*/

public SQLWarning
getWarnings()
    throws SQLException
{
    return( (conn != null) ? conn.getWarnings() : null );
} // getWarnings


/*
** Name: clearWarnings
**
** Description:
**	Clear the connection warning/message list.
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
**	29-Jan-01 (gordy)
**	    Created.
*/

public void
clearWarnings()
    throws SQLException
{
    if ( conn != null )  conn.clearWarnings();
    return;
} // clearWarnings


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
**	29-Jan-01 (gordy)
**	    Created.
*/

public Statement
createStatement()
    throws SQLException
{
    Statement stmt;
    checkConn( ".createStatement()" );

    try { stmt = conn.createStatement(); }
    catch( SQLException ex )
    {
	if ( conn.isClosed() )  errorEvent( ex );
	throw ex;
    }

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
**	29-Jan-01 (gordy)
**	    Created.
*/

public Statement
createStatement( int type, int concurrency )
    throws SQLException
{
    Statement stmt;
    checkConn( ".createStatement()" );

    try { stmt = conn.createStatement( type, concurrency ); }
    catch( SQLException ex )
    {
	if ( conn.isClosed() )  errorEvent( ex );
	throw ex;
    }

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
**	29-Jan-01 (gordy)
**	    Created.
*/

public PreparedStatement
prepareStatement( String sql )
    throws SQLException
{
    PreparedStatement stmt;
    checkConn( ".prepareStatement()" );

    try { stmt = conn.prepareStatement( sql ); }
    catch( SQLException ex )
    {
	if ( conn.isClosed() )  errorEvent( ex );
	throw ex;
    }

    return( stmt );
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
**	29-Jan-01 (gordy)
**	    Created.
*/

public PreparedStatement
prepareStatement( String sql, int type, int concurrency )
    throws SQLException
{
    PreparedStatement stmt;
    checkConn( ".prepareStatement()" );

    try { stmt = conn.prepareStatement( sql, type, concurrency ); }
    catch( SQLException ex )
    {
	if ( conn.isClosed() )  errorEvent( ex );
	throw ex;
    }

    return( stmt );
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
**	29-Jan-01 (gordy)
**	    Created.
*/

public CallableStatement 
prepareCall( String sql )
    throws SQLException
{
    CallableStatement stmt;
    checkConn( ".prepareCall()" );

    try { stmt = conn.prepareCall( sql ); }
    catch( SQLException ex )
    {
	if ( conn.isClosed() )  errorEvent( ex );
	throw ex;
    }

    return( stmt );
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
**	29-Jan-01 (gordy)
**	    Created.
*/

public CallableStatement 
prepareCall( String sql, int type, int concurrency )
    throws SQLException
{
    CallableStatement stmt;
    checkConn( ".prepareCall()" );

    try { stmt = conn.prepareCall( sql, type, concurrency ); }
    catch( SQLException ex )
    {
	if ( conn.isClosed() )  errorEvent( ex );
	throw ex;
    }

    return( stmt );
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
**	29-Jan-01 (gordy)
**	    Created.
*/

public boolean
getAutoCommit()
    throws SQLException
{
    boolean autoCommit;
    checkConn( ".getAutoCommit()" );

    try { autoCommit = conn.getAutoCommit(); }
    catch( SQLException ex )
    {
	if ( conn.isClosed() )  errorEvent( ex );
	throw ex;
    }

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
**	29-Jan-01 (gordy)
**	    Created.
*/

public void
setAutoCommit( boolean autoCommit )
    throws SQLException
{
    checkConn( ".setAutoCommit()" );

    try { conn.setAutoCommit( autoCommit ); }
    catch( SQLException ex )
    {
	if ( conn.isClosed() )  errorEvent( ex );
	throw ex;
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
**	29-Jan-01 (gordy)
**	    Created.
*/

public void
commit()
    throws SQLException
{
    checkConn( ".commit()" );

    try { conn.commit(); }
    catch( SQLException ex )
    {
	if ( conn.isClosed() )  errorEvent( ex );
	throw ex;
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
**	29-Jan-01 (gordy)
**	    Created.
*/

public void
rollback()
    throws SQLException
{
    checkConn( ".rollback()" );

    try { conn.rollback(); }
    catch( SQLException ex )
    {
	if ( conn.isClosed() )  errorEvent( ex );
	throw ex;
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
**	29-Jan-01 (gordy)
**	    Created.
*/

public String
nativeSQL( String sql )
    throws SQLException
{
    checkConn( ".nativeSQL()" );

    try { sql = conn.nativeSQL( sql ); }
    catch( SQLException ex )
    {
	if ( conn.isClosed() )  errorEvent( ex );
	throw ex;
    }

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
**	29-Jan-01 (gordy)
**	    Created.
*/

public DatabaseMetaData
getMetaData()
    throws SQLException
{
    DatabaseMetaData metaData;
    checkConn( ".getMetaData()" );

    try { metaData = conn.getMetaData(); }
    catch( SQLException ex )
    {
	if ( conn.isClosed() )  errorEvent( ex );
	throw ex;
    }

    return( metaData );
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
**	29-Jan-01 (gordy)
**	    Created.
*/

public boolean
isReadOnly()
    throws SQLException
{
    boolean readOnly;
    checkConn( ".isReadOnly()" );

    try { readOnly = conn.isReadOnly(); }
    catch( SQLException ex )
    {
	if ( conn.isClosed() )  errorEvent( ex );
	throw ex;
    }

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
**	29-Jan-01 (gordy)
**	    Created.
*/

public synchronized void
setReadOnly( boolean readOnly )
    throws SQLException
{
    checkConn( ".setReadOnly()" );

    try { conn.setReadOnly( readOnly ); }
    catch( SQLException ex )
    {
	if ( conn.isClosed() )  errorEvent( ex );
	throw ex;
    }

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
**	29-Jan-01 (gordy)
**	    Created.
*/

public int
getTransactionIsolation()
    throws SQLException
{
    int level;
    checkConn( ".getTransactionIsolation()" );

    try { level = conn.getTransactionIsolation(); }
    catch( SQLException ex )
    {
	if ( conn.isClosed() )  errorEvent( ex );
	throw ex;
    }

    return( level );
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
**	29-Jan-01 (gordy)
**	    Created.
*/

public synchronized void
setTransactionIsolation( int level )
    throws SQLException
{
    checkConn( ".setTransactionIsolation()" );

    try { conn.setTransactionIsolation( level ); }
    catch( SQLException ex )
    {
	if ( conn.isClosed() )  errorEvent( ex );
	throw ex;
    }

    return;
} // setTransactionIsolation


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
**	29-Jan-01 (gordy)
**	    Created.
*/

public String
getCatalog()
    throws SQLException
{
    String catalog;
    checkConn( ".getCatalog()" );

    try { catalog = conn.getCatalog(); }
    catch( SQLException ex )
    {
	if ( conn.isClosed() )  errorEvent( ex );
	throw ex;
    }

    return( catalog );
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
**	29-Jan-01 (gordy)
**	    Created.
*/

public void
setCatalog( String catalog )
    throws SQLException
{
    checkConn( ".setCatalog()" );

    try { conn.setCatalog( catalog ); }
    catch( SQLException ex )
    {
	if ( conn.isClosed() )  errorEvent( ex );
	throw ex;
    }

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
**	29-Jan-01 (gordy)
**	    Created.
*/

public Map
getTypeMap()
    throws SQLException
{
    Map map;
    checkConn( ".getTypeMap()" );

    try { map = conn.getTypeMap(); }
    catch( SQLException ex )
    {
	if ( conn.isClosed() )  errorEvent( ex );
	throw ex;
    }

    return( map );
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
**	29-Jan-01 (gordy)
*/

public void
setTypeMap( Map map )
    throws SQLException
{
    checkConn( ".setTypeMap()" );

    try { conn.setTypeMap( map ); }
    catch( SQLException ex )
    {
	if ( conn.isClosed() )  errorEvent( ex );
	throw ex;
    }

    return;
} // setTypeMap


/*
** Name: checkConn
**
** Description:
**	Make sure the connection is active.
**
** Input:
**	name	Callers name for tracing.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
*/

protected void
checkConn( String name )
    throws EdbcEx
{
    if ( conn == null )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + name + ": virtual connection closed!" );
	throw EdbcEx.get( E_JD0004_CONNECTION_CLOSED );
    }
    return;
} // checkConn


/*
** Name: closeEvent
**
** Description:
**	Raise a connection closed event.
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
**	29-Jan-01 (gordy)
**	    Created.
*/

private void
closeEvent()
{
    if ( trace.enabled( 2 ) )
	trace.write( tr_id + ": closing virtual connection" );

    if ( listener != null )
	listener.connectionClosed( new ConnectionEvent( pool ) );

    listener = null;
    pool = null;
    conn = null;
    return;
} // closeEvent


/*
** Name: errorEvent
**
** Description:
**	Raise a connection error event.
**
** Input:
**	ex	SQLException.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
**	12-Apr-01 (gordy)
**	    Exceptions are now self tracing.
*/

private void
errorEvent( SQLException ex )
{
    if ( trace.enabled() )
    {
	trace.log( title + ": virtual connection error" );
	if ( trace.enabled( 1 ) )  ((EdbcEx)ex).trace( trace );
    }

    if ( listener != null )
	listener.connectionErrorOccurred( new ConnectionEvent( pool, ex ) );

    listener = null;
    pool = null;
    conn = null;
    return;
} // errorEvent


} // class EdbcCPVirtConn
