/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbc;

/*
** Name: EdbcStmt.java
**
** Description:
**	Implements the EDBC JDBC Statement class: EdbcStmt.
**
** History:
**	13-May-99 (gordy)
**	    Created.
**	 8-Sep-99 (gordy)
**	    Created new base class for class which interact with
**	    the JDBC server and extracted common data and methods.
**	    Synchronize entire request/response with JDBC server.
**	13-Sep-99 (gordy)
**	    Implemented error code support.
**	29-Sep-99 (gordy)
**	    Use DbConn lock()/unlock() methods for synchronization
**	    to support BLOB streams.
**	26-Oct-99 (gordy)
**	    Implemented ODBC escape parsing and support for cursor
**	    positioned DELETE/UPDATE.
**	12-Nov-99 (gordy)
**	    Use configured date formatter.
**	16-Nov-99 (gordy)
**	    Added query timeouts.
**	13-Dec-99 (gordy)
**	    Added fetch limit.
**	16-May-00 (gordy)
**	    Move SQL parsing back into individual query processing
**	    sections so that it occurs after parser.getCursonName().
**	19-May-00 (gordy)
**	    Queries may be executed directly rather than opening a
**	    cursor (if enabled).
**	 4-Oct-00 (gordy)
**	    Standardized internal tracing.
**	31-Oct-00 (gordy)
**	    Added support for JDBC 2.0 extensions.
**	 5-Jan-01 (gordy)
**	    Added support for batch processing.
**	23-Jan-01 (gordy)
**	    Allocate batch list on first access.
**	23-Feb-01 (gordy)
**	    Removed finalize().
**	28-Mar-01 (gordy)
**	    Gave package access to some fields so internal access
**	    won't trigger tracing in access methods.  Same was done
**	    for connection fields.  For this same reason, call shut()
**	    in result-set rather than close().
**	20-Jun-01 (gordy)
**	    Pass cursor name to result-set according to JDBC spec.
**	21-Jun-01 (gordy)
**	    Reworked parsing to remove redundant code, transform
**	    'SELECT FOR UPDATE' and flag 'FOR UPDATE'.
**	20-Aug-01 (gordy)
**	    Support UPDATE concurrency.  Send READONLY query flag to server.
**	25-Oct-01 (gordy)
**	    Select loops are only supported at PROTO_2 and above.
*/

import	java.util.LinkedList;
import	java.util.NoSuchElementException;
import	java.sql.Connection;
import	java.sql.Statement;
import	java.sql.ResultSet;
import	java.sql.SQLException;
import  java.sql.BatchUpdateException;
import	ca.edbc.util.EdbcEx;
import	ca.edbc.io.DbConn;


/*
** Name: EdbcStmt
**
** Description:
**	EDBC JDBC Java driver class which implements the JDBC 
**	Statement interface.
**
**  Constants
**
**	QUERY		    Statement is a query.
**	UPDATE		    Statement is an update.
**	UNKNOWN		    Statement is unknown.
**	crsr_prefix	    Prefix for generated cursor names.
**	stmt_prefix	    Prefix for generated statement names.
**
**  Interface Methods:
**
**	executeQuery	    Execute a SQL query which returns a result set.
**	executeUpdate	    Execute a SQL query which may return a row count.
**	close		    Close the statement and release resources.
**	getMaxFieldSize	    Retrieve the maximum column length.
**	setMaxFieldSize	    Set the maximum column length.
**	getMaxRows	    Retrieve the maximum result set length.
**	setMaxRows	    Set the maximum result set length.
**	setEscapeProcessing Set the escape syntax processing state.
**	getQueryTimeout	    Retrieve the query result timeout.
**	setQueryTimeout	    Set the query result timeout.
**	cancel		    Cancel the execution of a query.
**	setCursorName	    Set the cursor name associated with the query.
**	execute		    Execute a SQL query with unknown result type.
**	getResultSet	    Retrieve next query result set.
**	getUpdateCount	    Retrieve next query result row count.
**	getMoreResults	    Determine next query result type.
**	setFetchDirection   Set the cursor fetch direction.
**	getFetchDirection   Retrieve the cursor fetch direction.
**	setFetchSize	    Set the cursor pre-fetch size.
**	getFetchSize	    Retrieve the cursor pre-fetch size.
**	getResultSetConcurrency	Retrieve the default ResultSet concurrency.
**	getResultSetType    Retrieve the default ResultSet type.
**	addBatch	    Add SQL request to batch set.
**	clearBatch	    Clear the batch set.
**	executeBatch	    Execute SQL requests in batch set.
**	getConnection	    Retrieve the associated Connection.
**
**  Protected Methods:
**
**	getConcurrency	    Resolve statement concurrency.
**	readDesc	    Read query result data descriptor.
**
**  Protected Data
**
**	connection	    Associated connection.
**	batch		    List of queries to be batched processed.
**	resultSet	    Current query result set.
**	crsr_name	    Cursor name.
**	parse_escapes	    True if escape syntax should be translated.
**	rs_type		    Default ResultSet type.
**	rs_concur	    Default ResultSet concurrency.
**	rs_fetch_dir	    ResultSet fetch direction.
**	rs_fetch_size	    ResultSet pre-fetch size, 0 for unspecified.
**	rs_max_rows	    Maximum rows in a result set, 0 for unlimited.
**	rs_max_len	    Maximum length of a column, 0 for unlimited.
**	timeout		    Query result timeout, 0 for no timeout.
**	inst_id		    Instance ID for tracing.
**
**  Private Methods:
**
**	exec		    Execute a statement.
**
** History:
**	13-May-99 (gordy)
**	    Created.
**	 8-Sep-99 (gordy)
**	    Created new base class for class which interact with
**	    the JDBC server and extracted common data and methods.
**	    Synchronize entire request/response with JDBC server.
**	26-Oct-99 (gordy)
**	    Combined bulk of executeQuery(), executeUpdate() and
**	    execute() code into single internal method.
**	 4-Oct-00 (gordy)
**	    Added instance count, inst_count, and instance ID,
**	    inst_id, for standardized internal tracing.
**	31-Oct-00 (gordy)
**	    Support JDBC 2.0 extensions.  Added conn, rs_type, rs_concur,
**	    rs_fetch_dir, rs_fetch_size, getConnection(), getResultSetType(), 
**	    getResultSetConcurrency(), getFetchDirection(), getFetchSize(), 
**	    setFetchDirection(), setFetchSize(), addBatch(), clearBatch(), 
**	    executeBatch().  Renamed max_row_cnt to rs_max_rows and 
**	    max_col_len to rs_max_len.
**	 5-Jan-01 (gordy)
**	    Added support for batch processing.  Added batch.
**	23-Jan-01 (gordy)
**	    The batch field cannot be allocated until a batch request is
**	    made so as to remain compatible with JDK 1.1 environments.
**	    LinkedList is a Java 2.0 class and should only be referenced
**	    in a JDK 1.3 environment.  Since the JDBC 2.0 extensions for
**	    batch processing should only be accessed in JDK 1.3, is should
**	    be sufficient to reserve allocation to a batch method.
**	23-Feb-01 (gordy)
**	    Removed finalize() method.  Associated result-set does not need 
**	    to be explicitly closed when this statement is no longer used.  
**	    The result-set may still be in use, even if the statement is no 
**	    longer referenced.  The EdbcRslt class has it's own finalize() 
**	    method to assure closure.
**	28-Mar-01 (gordy)
**	    Gave package access to connection (renamed from conn),
**	    rs_fetch_size, rs_max_rows, rs_max_len so they can be
**	    access internally without triggering tracing.
**	20-Aug-01 (gordy)
**	    Added query type constants and getConcurrency() method.
**	    Changed cursor to crsr_name for consistency.
*/

class
EdbcStmt
    extends	EdbcObj
    implements	Statement
{

    protected static final int	QUERY = 1;
    protected static final int	UPDATE = 0;
    protected static final int	UNKNOWN = -1;

    protected static final String   crsr_prefix = "JDBC_CRSR_";
    protected static final String   stmt_prefix = "JDBC_STMT_";

    /*
    ** The following require package access so that result-sets can
    ** access the info without triggering tracing or worrying about 
    ** exceptions.
    */
    EdbcConnect			connection;
    int				rs_fetch_size = 0;
    int				rs_max_rows = 0;
    int				rs_max_len = 0;

    protected LinkedList	batch = null;
    protected EdbcRslt		resultSet = null;
    protected String		crsr_name = null;
    protected int		rs_type;
    protected int		rs_concur;
    protected int		rs_fetch_dir = ResultSet.FETCH_FORWARD;
    protected boolean		parse_escapes = true;
    protected int		timeout = 0;
    protected int		inst_id;

    /*
    ** Number of EdbcStmt instances created.
    ** Used to create unique tracing ID.
    */
    private static int	inst_count = 0;


/*
** Name: EdbcStmt
**
** Description:
**	Class constructor.
**
** Input:
**	connection    Associated connection.
**	rs_type	    Default ResultSet type.
**	rs_concur   Default ResultSet concurrency.
**
** Output:
**	None.
**
** History:
**	13-May-99 (gordy)
**	    Created.
**	13-Jun-00 (gordy)
**	    Removed throws clause.
**	 4-Oct-00 (gordy)
**	    Create unique tracing ID for standardized internal tracing.
**	31-Oct-00 (gordy)
**	    Changed parameters for JDBC 2.0 extensions.
**	28-Mar-01 (gordy)
**	    Connection fields now have package access instead of methods.
*/

// package access needed
EdbcStmt( EdbcConnect connection, int rs_type, int rs_concur )
{
    super( connection.dbc, connection.trace );
    this.connection = connection;
    this.rs_type = rs_type;
    this.rs_concur = rs_concur;
    inst_id = inst_count++;
    title = shortTitle + "-Statement[" + inst_id + "]";
    tr_id = "Stmt[" + inst_id + "]";
} // EdbcStmt


/*
** Name: executeQuery
**
** Description:
**	Execute an SQL statement and return a result set.
**
** Input:
**	sql	    The query to be executed.
**
** Output:
**	None.
**
** Returns:
**	ResultSet   A new ResultSet object.
**
** History:
**	13-May-99 (gordy)
**	    Created.
**	 8-Sep-99 (gordy)
**	    Synchronize on DbConn.
**	29-Sep-99 (gordy)
**	    Use DbConn lock()/unlock() methods for synchronization.
**	26-Oct-99 (gordy)
**	    Call internal method to share code in conjunction with
**	    support for SQL parsing.
**	12-Nov-99 (gordy)
**	    Use configured date formatter.
**	21-Jun-01 (gordy)
**	    Simplified exec() parameters.
*/

public ResultSet
executeQuery( String sql )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".executeQuery( '" + sql + "' )" );
    
    exec( sql, QUERY );
    
    if ( trace.enabled() )  
	trace.log( title + ".executeQuery(): " + resultSet );
    return( resultSet );
} // executeQuery


/*
** Name: executeUpdate
**
** Description:
**	Execute an SQL statement and return a row count.
**
** Input:
**	sql	    The query to be executed.
**
** Output:
**	None.
**
** Returns:
**	int	    Row count or 0.
**
** History:
**	13-May-99 (gordy)
**	    Created.
**	 8-Sep-99 (gordy)
**	    Synchronize on DbConn.
**	29-Sep-99 (gordy)
**	    Use DbConn lock()/unlock() methods for synchronization.
**	26-Oct-99 (gordy)
**	    Call internal method to share code in conjunction with
**	    support for SQL parsing.
**	12-Nov-99 (gordy)
**	    Use configured date formatter.
**	21-Jun-01 (gordy)
**	    Simplified exec() parameters.
*/

public int
executeUpdate( String sql )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".executeUpdate( '" + sql + "' )" );
    
    exec( sql, UPDATE );
    
    if ( trace.enabled() )  
	trace.log( title + ".executeUpdate(): " + updateCount );
    return( updateCount < 0 ? 0 : updateCount );
} // executeUpdate


/*
** Name: execute
**
** Description:
**	Execute an SQL statement and return an indication as to the
**	type of the result.
**
** Input:
**	sql	    The query to be executed.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if the result is a ResultSet, false otherwise.
**
** History:
**	13-May-99 (gordy)
**	    Created.
**	 8-Sep-99 (gordy)
**	    Synchronize on DbConn.
**	29-Sep-99 (gordy)
**	    Use DbConn lock()/unlock() methods for synchronization.
**	26-Oct-99 (gordy)
**	    Call internal method to share code in conjunction with
**	    support for SQL parsing.
**	12-Nov-99 (gordy)
**	    Use configured date formatter.
**	21-Jun-01 (gordy)
**	    Simplified exec() parameters.
*/

public boolean 
execute( String sql )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".execute( '" + sql + "' )" );
    
    exec( sql, UNKNOWN );

    if ( trace.enabled() )  
	trace.log( title + ".execute(): " + (resultSet != null) );
    return( resultSet != null );
} // execute


/*
** Name: getResultSet
**
** Description:
**	Returns the result set produced by an execute() request.
**	If the current result is not a result set, null is returned.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	ResultSet   The next result set, or null.
**
** History:
**	13-May-99 (gordy)
**	    Created.
*/

public ResultSet
getResultSet()
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getResultSet(): " + resultSet );
    return( resultSet );
} // getResultSet


/*
** Name: getUpdateCount
**
** Description:
**	Returns the current result count.  If the current result is
**	a result set, -1 is returned.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Row count or -1.
**
** History:
**	13-May-99 (gordy)
**	    Created.
*/

public int
getUpdateCount()
    throws SQLException
{
    int updateCount = (resultSet != null) ? -1 : this.updateCount;
    
    if ( trace.enabled() )  
	trace.log( title + ".getUpdateCount(): " + updateCount );
    return( updateCount );
} // getUpdateCount


/*
** Name: getMoreResults
**
** Description:
**	Returns an indication that the next result is a result set.
**
**	We only have a single result value, so we just clear the current
**	results and return false.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if next result is a result set, false otherwise.
**
** History:
**	13-May-99 (gordy)
**	    Created.
*/

public boolean
getMoreResults()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getMoreResults(): " + false );
    clearResults();
    return( false );
} // getMoreResults


/*
** Name: clearResults
**
** Description:
**	Clear any existing query results.
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
**	13-May-99 (gordy)
**	    Created.
**	 6-Mar-01 (gordy)
**	    Warnings are cleared by call to super.
**	28-Mar-01 (gordy)
**	    Call result-set shut() method to avoid top-level tracing.
*/

protected synchronized void
clearResults()
{
    if ( resultSet != null )
    {
	try { resultSet.shut(); }
	catch( SQLException ignore ) {}
	resultSet = null;
    }

    super.clearResults();
    return;
}


/*
** Name: addBatch
**
** Description:
**	Add query to batch set.  
**
**	Synchronizes on the batch list so no other thread
**	can do batch operations during execution.
**
** Input:
**	sql	Query to be added to batch set.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	31-Oct-00 (gordy)
**	    Created.
**	 5-Jan-01 (gordy)
**	    Implemented.
**	23-Jan-01 (gordy)
**	    Allocate batch list on first access.
*/

public void
addBatch( String sql )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".addBatch( '" + sql + "' )" );
    if ( batch == null )  synchronized( this ) { batch = new LinkedList(); }
    synchronized( batch ) { batch.addLast( sql ); }
    return;
} // addBatch


/*
** Name: clearBatch
**
** Description:
**	Clear the batch set.  
**
**	Synchronizes on the batch list so no other thread
**	can do batch operations during execution.
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
**	31-Oct-00 (gordy)
**	    Created.
**	 5-Jan-01 (gordy)
**	    Implemented.
**	23-Jan-01 (gordy)
**	    Allocate batch list on first access.
*/

public void
clearBatch()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".clearBatch()" );
    if ( batch == null )  synchronized( this ) { batch = new LinkedList(); }
    synchronized( batch ) { batch.clear(); }
    return;
} // clearBatch


/*
** Name: executeBatch
**
** Description:
**	Execute the queries in the batch set.
**
**	Synchronizes on the batch list so no other thread
**	can do batch operations during execution.  The
**	connection is locked for each individual query in
**	the batch, so other threads can intermix other
**	queries during the batch execution.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int []	    Array of update counts for each query.
**
** History:
**	31-Oct-00 (gordy)
**	    Created.
**	 5-Jan-01 (gordy)
**	    Implemented.
**	23-Jan-01 (gordy)
**	    Allocate batch list on first access.
**	21-Jun-01 (gordy)
**	    Simplified exec() parameters.
*/

public int []
executeBatch()
    throws SQLException
{
    int results[];

    if ( trace.enabled() )  trace.log( title + ".executeBatch()" );
    if ( batch == null )  synchronized( this ) { batch = new LinkedList(); }

    synchronized( batch )
    {
	int count = batch.size();
	results = new int[ count ];

	/*
	** Execute each individual query in the batch.
	*/
	for( int i = 0; i < count; i++ )
	    try
	    {
		String sql = (String)batch.removeFirst();
		if ( trace.enabled() )  
		    trace.log( title + ".executeBatch[" + i + "]: '" 
				     + sql + "'" );

		exec( sql, UPDATE );
		results[ i ] = (updateCount < 0) ? -2 : updateCount;

		if ( trace.enabled() )  
		    trace.log( title + ".executeBatch[" + i + "] = " 
				     + results[ i ] );
	    }
	    catch( NoSuchElementException e )	// Shouldn't happen
	    {
		/*
		** Return only the successful query results.
		*/
		int temp[] = new int[ i ];
		if ( i > 0 )  System.arraycopy( results, 0, temp, 0, i );
		results = temp;
		break;
	    }
	    catch( SQLException e )
	    {
		/*
		** Return only the successful query results.
		*/
		int temp[] = new int[ i ];
		if ( i > 0 )  System.arraycopy( results, 0, temp, 0, i );

		/*
		** Build a batch exception from the sql exception.
		*/
		BatchUpdateException bue;
		bue = new BatchUpdateException( e.getMessage(), e.getSQLState(),
						e.getErrorCode(), temp );
		bue.setNextException( e.getNextException() );
		batch.clear();
		throw bue;
	    }
	
	batch.clear();
    }

    return( results );
} // executeBatch


/*
** Name: exec
**
** Description:
**	Send SQL statement to server and process results.
**	A cursor is opened when executing a SELECT and a
**	result-set is generated.  Non-SELECT statements
**	are simply executed and the row count is updated.
**
** Input:
**	text	    Statement text.
**	mode	    Execute mode: QUERY, UPDATE, UNKNOWN.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	26-Oct-99 (gordy)
**	    Extracted from interface methods.
**	15-Nov-99 (gordy)
**	    Pass max row count and column length to result set.
**	16-Nov-99 (gordy)
**	    Added query timeouts.
**	13-Dec-99 (gordy)
**	    Added fetch limit.
**	16-May-00 (gordy)
**	    Move SQL parsing back into individual query processing
**	    sections so that it occurs after parser.getCursonName().
**	19-May-00 (gordy)
**	    If select loops have been enabled, and a cursor name has
**	    not been assigned, execute queries directly rather than
**	    opening a cursor.  Locking had to be reorganized since
**	    select loops are not unlocked until the result set is
**	    closed.  Also needed additional handling when executing
**	    (supposedly) non-queries and get a result set by mistake.
**	28-Mar-01 (gordy)
**	    Call result-set shut() to avoid top-level tracing.
**	20-Jun-01 (gordy)
**	    Pass cursor name to result-set according to JDBC spec:
**	    NULL if READONLY and not provided by application.
**	21-Jun-01 (gordy)
**	    Re-worked parsing to eliminate redundant code.  Parser
**	    parameter replaced with simple query text string.  Query
**	    parameter replaced with mode to convey caller intent.
**	20-Aug-01 (gordy)
**	    Use cursor type constants.  Send READONLY flag to server.
**	    Warn if cursor mode changed to READONLY.
**	25-Oct-01 (gordy)
**	    Select loops are only supported at PROTO_2 and above.
**	31-Oct-01 (gordy)
**	    Timezone now passed to EdbcSQL.
*/

private void
exec( String text, int mode )
    throws EdbcEx
{
    EdbcSQL	parser = new EdbcSQL( text, getConnDateFormat().getTimeZone() );
    int		type = parser.getQueryType();
    EdbcRSMD	rsmd;
    String	cursor = null;
    boolean	cursor_op;

    clearResults();
    if ( mode == UNKNOWN )  mode = (type == EdbcSQL.QT_SELECT) ? QUERY : UPDATE;
    cursor_op = ( mode != QUERY  &&  
		  (type == EdbcSQL.QT_DELETE || type == EdbcSQL.QT_UPDATE)  &&
		  (cursor = parser.getCursorName()) != null
		) ? true : false;

    dbc.lock();
    try
    {	
	String	sql = parser.parseSQL( parse_escapes );

	if ( mode == QUERY )
	{
	    int	concurrency = getConcurrency( parser.getConcurrency() );

	    if ( 
	         dbc.select_loops  &&  crsr_name == null  &&  
		 concurrency != EdbcSQL.CONCUR_UPDATE  &&
		 dbc.msg_protocol_level >= JDBC_MSG_PROTO_2
	       )
	    {
		/*
		** Select statement using a select loop.
		*/
		dbc.begin( JDBC_MSG_QUERY );
		dbc.write( JDBC_QRY_EXQ );
		dbc.write( JDBC_QP_QTXT );
		dbc.write( sql );
		dbc.done( true );

		if ( (rsmd = readResults( timeout )) == null )
		    throw EdbcEx.get( E_JD0008_NOT_QUERY );

		resultSet = new RsltSlct( this, rsmd, stmt_id, fetchSize );
	    }
	    else
	    {
		/*
		** Select statement using a cursor.  Generate a 
		** cursor ID if not provided by the application.
		*/
		cursor = (crsr_name != null) 
			 ? crsr_name : dbc.getUniqueID( crsr_prefix );

		dbc.begin( JDBC_MSG_QUERY );
		dbc.write( JDBC_QRY_OCQ );
		dbc.write( JDBC_QP_CRSR_NAME );
		dbc.write( cursor );

		/*
		** Parser removes FOR READONLY clause because it isn't
		** a part of Ingres SELECT syntax.  Tell server that
		** cursor should be READONLY (kludge older protocol
		** by restoring clause to query).
		*/
		if ( concurrency == EdbcSQL.CONCUR_READONLY )
		    if ( dbc.msg_protocol_level < JDBC_MSG_PROTO_2 )
			sql += " for readonly";
		    else
		    {
			dbc.write( JDBC_QP_FLAGS );
			dbc.write( (short)2 );
			dbc.write( JDBC_QF_READONLY );
		    }

		dbc.write( JDBC_QP_QTXT );
		dbc.write( sql );
		dbc.done( true );

		if ( (rsmd = readResults( timeout )) == null )
		    throw EdbcEx.get( E_JD0008_NOT_QUERY );

		/*
		** Updatable cursors fetch only one row at a time.
		** Readonly cursors use the pre-fetch size returned
		** by the server.  The cursor name is passed to the
		** result set for updatable cursors or if provided
		** by the application (JDBC 2.1 API spec).
		*/
		if ( ! cursorReadonly )  
		    fetchSize = 1;
		else
		{
		    cursor = crsr_name;

		    if ( rs_concur == EdbcSQL.CONCUR_UPDATE )
			warnings = EdbcEx.getWarning( E_JD0017_RS_CHANGED );
		}

		resultSet = new RsltCurs( this,rsmd,stmt_id,cursor,fetchSize );
		dbc.unlock(); 
	    }
	}
	else  if ( cursor_op )
	{
	    /*
	    ** Positioned Delete/Update.
	    */
	    dbc.begin( JDBC_MSG_QUERY );
	    dbc.write( (type == EdbcSQL.QT_DELETE) ? JDBC_QRY_CDEL 
						   : JDBC_QRY_CUPD );
	    dbc.write( JDBC_QP_CRSR_NAME );
	    dbc.write( cursor );
	    dbc.write( JDBC_QP_QTXT );
	    dbc.write( sql );
	    dbc.done( true );

	    if ( readResults( timeout ) != null )   /* shouldn't happen */
		throw EdbcEx.get( E_JD0009_NOT_UPDATE );
	    dbc.unlock(); 
	}
	else
	{
	    /*
	    ** Non-query statement.
	    */
	    dbc.begin( JDBC_MSG_QUERY );
	    dbc.write( JDBC_QRY_EXQ );
	    dbc.write( JDBC_QP_QTXT );
	    dbc.write( sql );
	    dbc.done( true );

	    if ( (rsmd = readResults( timeout )) == null )
		dbc.unlock();		// We be done.
	    else
	    {
		/*
		** Query unexpectedly returned a result-set.
		** We need to cleanup the tuple stream and
		** statement in the JDBC server.  The easiest
		** way to do this is go ahead and create a
		** result-set and close it.
		*/
		resultSet = new RsltSlct( this, rsmd, stmt_id, 1 );
		try { resultSet.shut(); }
		catch( SQLException ignore ) {}
		resultSet = null;
		throw EdbcEx.get( E_JD0009_NOT_UPDATE );
	    }
	}
    }
    catch( EdbcEx ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".execute(): error executing query" );
	if ( trace.enabled( 1 ) )  ex.trace( trace );
	dbc.unlock(); 
	throw ex;
    }

    return;
} // exec


/*
** Name: cancel
**
** Description:
**	Interrupt and cancel the execution of a query.
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
**	13-May-99 (gordy)
**	    Created.
*/

public void
cancel()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".cancel()" );
    dbc.cancel();
    return;
} // cancel


/*
** Name: close
**
** Description:
**	Close a statement and release associated resources.
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
**	13-May-99 (gordy)
**	    Created.
*/

public void
close()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".close()" );
    clearResults();
    return;
} // close


/*
** Name: getConnection
**
** Description:
**	Retrieve the associated connection.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Connection  The associated connection.
**
** History:
**	31-Oct-00 (gordy)
**	    Created.
*/

public Connection
getConnection()
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getConnection(): " + connection );
    return( connection );
} // getConnection


/*
** Name: getResultSetType
**
** Description:
**	Retrieves the default ResultSet type.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Default ResultSet type.
**
** History:
**	31-Oct-00 (gordy)
**	    Created.
*/

public int
getResultSetType()
{
    if ( trace.enabled() )  
	trace.log( title + ".getResultSetType(): " + rs_type );
    return( rs_type );
} // getResultSetType


/*
** Name: getResultSetConcurrency
**
** Description:
**	Retrieves the default ResultSet concurrency.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Default ResultSet concurrency.
**
** History:
**	31-Oct-00 (gordy)
**	    Created.
*/

public int
getResultSetConcurrency()
{
    if ( trace.enabled() )  
	trace.log( title + ".getResultSetConcurrency(): " + rs_concur );
    return( rs_concur );
} // getResultSetConcurrency


/*
** Name: setCursorName
**
** Description:
**	Set the cursor name associated with the query.
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
**	13-May-99 (gordy)
**	    Created.
*/

public void
setCursorName( String name )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setCursorName( '" + name + "' )" );
    crsr_name = name;
    return;
} // setCursorName


/*
** Name: setEscapeProcessing
**
** Description:
**	Set the escape syntax processing state.
**
** Input:
**	enable	    True to process escape syntax, false to ignore.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	13-May-99 (gordy)
**	    Created.
*/

public void
setEscapeProcessing( boolean enable )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setEscapeProcessing( " + enable + " )" );
    parse_escapes = enable;
    return;
} // setEscapeProcessing


/*
** Name: getFetchDirection
**
** Description:
**	Retrieves the ResultSet fetch direction.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Fetch direction.
**
** History:
**	31-Oct-00 (gordy)
**	    Created.
*/

public int
getFetchDirection()
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getFetchDirection(): " + rs_fetch_dir );
    return( rs_fetch_dir );
} // getFetchDirection


/*
** Name: setFetchDirection
**
** Description:
**	Set the ResultSet fetch direction.
**
** Input:
**	dir	Fetch direction
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	31-Oct-00 (gordy)
**	    Created.
*/

public void
setFetchDirection( int dir )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setFetchDirection( " + dir + " )" );

    switch( dir )
    {
    case ResultSet.FETCH_FORWARD :
    case ResultSet.FETCH_REVERSE :
    case ResultSet.FETCH_UNKNOWN :
	break;

    default :
	throw EdbcEx.get( E_JD000A_PARAM_VALUE );
    }

    rs_fetch_dir = dir;
    return;
} // setFetchDirection


/*
** Name: getFetchSize
**
** Description:
**	Retrieves the suggested ResultSet pre-fetch size.
**	A 0 value indicates no specific size.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Pre-fetch size, 0 for unspecified.
**
** History:
**	31-Oct-00 (gordy)
**	    Created.
*/

public int
getFetchSize()
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getFetchSize(): " + rs_fetch_size );
    return( rs_fetch_size );
} // getFetchSize


/*
** Name: setFetchSize
**
** Description:
**	Set the suggested ResultSet pre-fetch size.  A 0 value
**	indicates no specific size.
**
** Input:
**	rows	Pre-fetch size..
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	31-Oct-00 (gordy)
**	    Created.
*/

public void
setFetchSize( int rows )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setFetchSize( " + rows + " )" );

    if ( rows < 0  ||  (rs_max_rows > 0  &&  rows > rs_max_rows) )
	throw EdbcEx.get( E_JD000A_PARAM_VALUE );

    rs_fetch_size = rows;
    return;
} // setFetchSize


/*
** Name: getMaxRows
**
** Description:
**	Retrieve the maximum number of rows which may be returned
**	by a result set.  Zero is returned if there is no limit.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	The result set maximum row limit, 0 for unlimited.
**
** History:
**	13-May-99 (gordy)
**	    Created.
*/

public int
getMaxRows()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getMaxRows(): " + rs_max_rows );
    return( rs_max_rows );
} // getMaxRows

/*
** Name: setMaxRows
**
** Description:
**	Set the maximum number of rows which will be returned in
**	a result set.  Use zero for unlimited rows.
**
** Input:
**	max	    Result set maximum row limit.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	13-May-99 (gordy)
**	    Created.
**      16-Mar-01 (loera01) Bug 104333
**	    Throw an exception if the input parameter is negative.
*/

public void
setMaxRows( int max )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".setMaxRows( " + max + " )" );
    if ( max < 0 )
        throw EdbcEx.get( E_JD000A_PARAM_VALUE );
    rs_max_rows = max;
    return;
} // setMaxRows


/*
** Name: getMaxFieldSize
**
** Description:
**	Get the maximum length of data which may be returned for a 
**	column.  Zero is returned if there is no limit.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Maximum column length, 0 for unlimited.
**
** History:
**	13-May-99 (gordy)
**	    Created.
*/

public int
getMaxFieldSize()
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getMaxFieldSize(): " + rs_max_len );
    return( rs_max_len );
} // getMaxFieldSize


/*
** Name: setMaxFieldSize
**
** Description:
**	Set the maximum length of data which may returned for a
**	column.  Use zero for unlimited length.
**
** Input:
**	max	Maximum column length.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	13-May-99 (gordy)
**	    Created.
**      16-Mar-01 (loera01) Bug 104333
**	    Throw an exception if the input parameter is negative.
*/

public void
setMaxFieldSize( int max )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setMaxFieldSize( " + max + " )" );
    if ( max < 0 )
        throw EdbcEx.get( E_JD000A_PARAM_VALUE );
    rs_max_len = max;
    return;
} // setMaxFieldSize


/*
** Name: getQueryTimeout
**
** Description:
**	Retrieve the query result timeout.  Zero indicates no timeout.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Timeout in seconds, 0 for no timeout.
**
** History:
**	13-May-99 (gordy)
**	    Created.
*/

public int
getQueryTimeout()
    throws SQLException
{
    if ( trace.enabled() )  trace.log(title + ".getQueryTimeout(): " + timeout);
    return( timeout );
} // getQueryTimeout

/*
** Name: setQueryTimemout
**
** Description:
**	Set the waiting for query result timeout.  Use zero for no timeout.
**
** Input:
**	seconds	    Timeout in seconds.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	13-May-99 (gordy)
**	    Created.
**      16-Mar-01 (loera01) Bug 104333
**	    Throw an exception if the input parameter is negative.
*/

public void
setQueryTimeout( int seconds )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setQueryTimeout( " + seconds + " )" );
    if ( seconds < 0 )
	throw EdbcEx.get( E_JD000A_PARAM_VALUE );
    timeout = seconds;
    return;
} // setQueryTimeout


/*
** Name: getConcurrency
**
** Description:
**	Returns the concurrency type for this statement based
**	on the SQL text, statement, connection, and default
**	concurrency settings.
**
** Input
**	concurrency	Concurrency of SQL text.
**
** Output:
**	None.
**
** Returns:
**	int		Resolved concurrency
**
** History:
**	20-Aug-01 (gordy)
**	    Created.
*/

protected int
getConcurrency( int concurrency )
{
    if ( concurrency != EdbcSQL.CONCUR_DEFAULT )  return( concurrency );
    if ( rs_concur != EdbcSQL.CONCUR_DEFAULT )  return( rs_concur );
    
    try { if ( connection.isReadOnly() )  return( EdbcSQL.CONCUR_READONLY ); }
    catch( SQLException ignore ) {}

    if ( dbc.cursor_mode == DbConn.CRSR_READONLY )  
	return( EdbcSQL.CONCUR_READONLY );
    else  if ( dbc.cursor_mode == DbConn.CRSR_UPDATE )
	return( EdbcSQL.CONCUR_UPDATE );

    return( EdbcSQL.CONCUR_DEFAULT );
} // getConcurrency


/*
** Name: readDesc
**
** Description:
**	Read a query result data descriptor message.  Overrides the
**	default method in EdbcObject.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	EdbcRSMD    Query result data descriptor.
**
** History:
**	 8-Sep-99 (gordy)
**	    Created.
*/

protected EdbcRSMD
readDesc()
    throws EdbcEx
{
    return( EdbcRSMD.load( dbc, trace ) );
} // readDesc

} // class EdbcStmt
