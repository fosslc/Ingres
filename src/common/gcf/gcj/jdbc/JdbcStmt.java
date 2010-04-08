/*
** Copyright (c) 1999, 2010 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: JdbcStmt.java
**
** Description:
**	Defines class which implements the JDBC Statement interface.
**
**  Classes:
**
**	JdbcStmt	Implements the JDBC Statement interface.
**	BatchExec	Batch query execution.
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
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	19-Feb-03 (gordy)
**	    Updated to JDBC 3.0.
**	15-Apr-03 (gordy)
**	    Added connection timezones separate from date formatters.
**	 3-Jul-03 (gordy)
**	    Added Ingres timezone config which affects date/time literals.
**	    Set DBMS types for generated keys to LOGKEY/TBLKEY.
**	 4-Aug-03 (gordy)
**	    New result-set for updatable cursor.  Simplified pre-fetch 
**	    calculations.
**	20-Aug-03 (gordy)
**	    Add statement AUTO_CLOSE optimization.
**	26-Sep-03 (gordy)
**	    Result-set data now stored as SqlData objects.
**	 6-Oct-03 (gordy)
**	    First block of rows may now be fetched with query response.
**	 1-Nov-03 (gordy)
**	    Implemented updatable result-sets.
**	 5-May-04 (gordy)
**	    Futher optimized prepared statements.
**	 9-Nov-06 (gordy)
**	    Handle query text over 64K.
**	15-Nov-06 (gordy)
**	    Support LOB Locators.
**	30-Jan-07 (gordy)
**	    Backward compatibility for LOB Locators.
**	26-Feb-07 (gordy)
**	    Additional configuration for LOB Locators.
**	 6-Apr-07 (gordy)
**	    Support scrollable cursors.
**	 4-May-07 (gordy)
**	    Set class access for reflection.
**	20-Jul-07 (gordy)
**	    Pre-fetching may be limited (-1), unlimited (0), or suggested.
**      05-Jan-09 (rajus01) SIR 121238
**          - Added new JDBC 4.0 methods to avoid compilation errors with
**            JDK 1.6. The new methods return E_GC4019 error for now.
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to
**            support JDBC 4.0 SQLException hierarchy.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	 3-Mar-09 (gordy)
**	    Add config setting to enable/disable scrollable cursors.
**	25-Mar-10 (gordy)
**	    Implement support for actual batch processing.
*/

import	java.util.LinkedList;
import	java.util.NoSuchElementException;
import	java.sql.Connection;
import	java.sql.Statement;
import	java.sql.ResultSet;
import	java.sql.Types;
import	java.sql.SQLException;
import  java.sql.BatchUpdateException;
import  java.sql.SQLWarning;
import	com.ingres.gcf.util.DbmsConst;
import	com.ingres.gcf.util.SqlData;
import	com.ingres.gcf.util.SqlByte;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.util.SqlWarn;
import	com.ingres.gcf.util.XaEx;
import	com.ingres.gcf.util.Timer;


/*
** Name: JdbcStmt
**
** Description:
**	JDBC driver class which implements the JDBC Statement interface.
**
**  Public Data:
**
**	rs_fetch_dir	    ResultSet fetch direction.
**	rs_fetch_size	    ResultSet pre-fetch size, 0 for unspecified.
**	rs_max_rows	    Maximum rows in a result set, 0 for unlimited.
**	rs_max_len	    Maximum length of a column, 0 for unlimited.
**
**  Interface Methods:
**
**	executeQuery		Execute SQL query which returns a result set.
**	executeUpdate		Execute SQL query which may return a row count.
**	execute			Execute SQL query with unknown result type.
**	getResultSet		Retrieve next query result set.
**	getUpdateCount		Retrieve next query result row count.
**	getMoreResults		Determine next query result type.
**	getGeneratedKeys	Retrieve table and/or object keys.
**	addBatch		Add SQL request to batch set.
**	clearBatch		Clear the batch set.
**	executeBatch		Execute SQL requests in batch set.
**	cancel			Cancel the execution of a query.
**	close			Close the statement and release resources.
**	getConnection		Retrieve the associated Connection.
**	getResultSetType	Retrieve the default ResultSet type.
**	getResultSetConcurrency	Retrieve the default ResultSet concurrency.
**	getResultSetHoldability	Retrieve the default ResultSet holdability.
**	setCursorName		Set the cursor name associated with the query.
**	setEscapeProcessing	Set the escape syntax processing state.
**	getFetchDirection	Retrieve the cursor fetch direction.
**	setFetchDirection	Set the cursor fetch direction.
**	getFetchSize		Retrieve the cursor pre-fetch size.
**	setFetchSize		Set the cursor pre-fetch size.
**	getMaxRows		Retrieve the maximum result set length.
**	setMaxRows		Set the maximum result set length.
**	getMaxFieldSize		Retrieve the maximum column length.
**	setMaxFieldSize		Set the maximum column length.
**	getQueryTimeout		Retrieve the query result timeout.
**	setQueryTimeout		Set the query result timeout.
**
**	timeExpired		Timer callback method.
**
**  Constants
**
**	QUERY			Statement is a query.
**	UPDATE			Statement is an update.
**	UNKNOWN			Statement is unknown.
**	crsr_prefix		Prefix for generated cursor names.
**	stmt_prefix		Prefix for generated statement names.
**
**  Protected Data
**
**	batch			List of queries to be batched processed.
**	resultSet		Current query result set.
**	crsr_name		Cursor name.
**	parse_escapes		True if escape syntax should be translated.
**	rs_type			Default ResultSet type.
**	rs_concur		Default ResultSet concurrency.
**	rs_hold			Default ResultSet holdability.
**	timeout			Query result timeout, 0 for no timeout.
**
**  Protected Methods:
**
**	clearResults		Clear all query results.
**	clearQueryResults	Clear update count and result-set.
**	newBatch		Allocate batch list.
**	getConcurrency		Resolve statement concurrency.
**	getPreFetchSize		Returns result-set fetch size.
**	enableLocators		Are LOB Locators permitted?
**	readResults		Read query results with timeout.
**	readDesc		Read query result data descriptor.
**	readError		Reqd query result error message.
**	readResult		Read query result done message.
**
**  Private Data:
**
**	rsmdKeys		Result-set meta-data for table and object keys.
**	rsmdTblKey		Result-set meta-data for table key.
**	rsmdObjKey		Result-set meta-data for object key.
**	rsmdEmpty		Result-set meta-data for no keys.
**	rsEmpty			Result-set for no keys.
**	timer			Statement execution timer.
**	timed_out		Time-out flag.
**	closed			Statement close flag.
**
**  Private Methods:
**
**	iterateBatch		Execute individual batch statements.
**	exec			Execute a statement.
**	enableTimer		Start timer for query timeout.
**	disableTimer		Disable query timer.
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
**	16-Nov-99 (gordy)
**	    Added query timeouts: timer, timed_out, timeExpired().
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
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.  Super-class no longer supports 
**	    timed statements so added readResults(), readError(), readDone(),
**	    timeExpired(), enableTimer(), disableTimer(), timer and timed_out.
**	    Added newBatch() to allocate batch list.  Removed instance count
**	    & id and used super-class instance id.
**	19-Feb-03 (gordy)
**	    Updated to JDBC 3.0.  Added rs_hold, getResultSetHoldability(),
**	    rsmdKeys, rsmdTblKey, rsmdObjKey, rsmdEmpty, rsEmpty,
**	    getGeneratedKeys(), getMoreResults(), getFetchSize().
**	 4-Aug-03 (gordy)
**	    Renamed internal getFetchSize() method due to name conflict
**	    with interface method when parameter list changed.  Pre-fetch 
**	    size calculations now based solely on available size information 
**	    and indepedent of other pre-fetch restrictions.
**	 6-Oct-03 (gordy)
**	    Added readResults() variant to handle interrupts from readResult(),
**	    which was renamed from readDone().  Our readResult() always does
**	    an interrupt which is only desired for result-set queries, so the
**	    new readResults() variant allows caller to request interrupts to
**	    be ignored.
**	 5-May-04 (gordy)
**	    Moved statement prefix to prepared statement class DrvPrep.
**	    Added readDesc() method for sub-classes to reload an existing
**	    RSMD (while providing timeout handling).
**	26-Feb-07 (gordy)
**	    Added enableLocators().
**	 4-May-07 (gordy)
**	    Class is exposed outside package, permit access.
**	25-Mar-10 (gordy)
**	    Moved individual batch statement execution to iterateBatch()
**	    and added BatchExec class for actual batch execution.
*/

public class
JdbcStmt
    extends	DrvObj
    implements	Statement, DbmsConst, Timer.Callback
{
    /*
    ** The following require package access so that result-sets can
    ** access the info without triggering tracing or worrying about 
    ** exceptions.
    */
    int					rs_fetch_dir = ResultSet.FETCH_FORWARD;
    int					rs_fetch_size = 0;
    int					rs_max_rows = 0;
    int					rs_max_len = 0;

    /*
    ** Constants
    */
    protected static final int		QUERY = 1;
    protected static final int		UPDATE = 0;
    protected static final int		UNKNOWN = -1;

    protected static final String	crsr_prefix = "JDBC_CRSR_";

    protected LinkedList		batch = null;
    protected JdbcRslt			resultSet = null;
    protected String			crsr_name = null;
    protected int			rs_type;
    protected int			rs_concur;
    protected int			rs_hold;
    protected boolean			parse_escapes = true;
    protected int			timeout = 0;

    /*
    ** Result-sets for returning auto generated keys.
    */
    private static JdbcRSMD		rsmdKeys = null;
    private static JdbcRSMD		rsmdTblKey = null;
    private static JdbcRSMD		rsmdObjKey = null;
    private static JdbcRSMD		rsmdEmpty = null;
    private static ResultSet		rsEmpty = null;

    /*
    ** The only thing we can do to timeout a query is
    ** set a timer locally and attempt to cancel the
    ** query if the time expires.  We also flag the
    ** timeout condition to translate the cancelled
    ** query error into a timeout error.
    */
    private Timer			timer = null;
    private boolean			timed_out = false;

    /*
    ** Flag to indicate the statement is closed (JDBC 4.0).
    */
    private boolean			closed = false;


/*
** Name: JdbcStmt
**
** Description:
**	Class constructor.
**
**	Caller is responsible for ensuring that ResultSet type,
**	concurrency, and holdability are suitable for connection.
**
** Input:
**	conn	    Associated connection.
**	rs_type	    Default ResultSet type.
**	rs_concur   Cursor concurrency.
**	rs_hold	    Default ResultSet holdability.
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
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	19-Feb-03 (gordy)
**	    Added default ResultSet holdability.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, restrict constructor access.
*/

// package access
JdbcStmt( DrvConn conn, int rs_type, int rs_concur, int rs_hold )
{
    super( conn );
    this.rs_type = rs_type;
    this.rs_concur = rs_concur;
    this.rs_hold = rs_hold;
    title = trace.getTraceName() + "-Statement[" + inst_id + "]";
    tr_id = "Stmt[" + inst_id + "]";
    return;
} // JdbcStmt


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
    if ( trace.enabled() )  trace.log(title + ".executeQuery('" + sql + "')");
    exec( sql, QUERY );
    if ( trace.enabled() )  trace.log(title + ".executeQuery(): " + resultSet);
    return( resultSet );
} // executeQuery


/*
** Name: executeUpdate
**
** Description:
**	Execute an SQL statement and return a row count.
**
**	Parameters related to auto generated keys are ignored.
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
**	21-Feb-03 (gordy)
**	    Added auto gen key methods.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Clean up tracing of array parameter.
*/

public int
executeUpdate( String sql )
    throws SQLException
{
    if ( trace.enabled() )  trace.log(title + ".executeUpdate('" + sql + "')");
    exec( sql, UPDATE );
    int count = ((rslt_items & RSLT_ROW_CNT) != 0) ? rslt_val_rowcnt : 0;
    if ( trace.enabled() )  trace.log( title + ".executeUpdate(): " + count );
    return( count );
} // executeUpdate


public int
executeUpdate( String sql, int autoGeneratedKeys )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".executeUpdate('" + 
					sql + "'," + autoGeneratedKeys + ")" );
    switch( autoGeneratedKeys )
    {
    case RETURN_GENERATED_KEYS :    break;
    case NO_GENERATED_KEYS :	    break;
    default : throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    }

    exec( sql, UPDATE );
    int count = ((rslt_items & RSLT_ROW_CNT) != 0) ? rslt_val_rowcnt : 0;
    if ( trace.enabled() )  trace.log( title + ".executeUpdate(): " + count );
    return( count );
} // executeUpdate


public int
executeUpdate( String sql, int columns[] )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".executeUpdate('" + sql + "'," + 
		   (columns == null ? "null" : "[" + columns.length + "]") + 
		   ")" );

    exec( sql, UPDATE );
    int count = ((rslt_items & RSLT_ROW_CNT) != 0) ? rslt_val_rowcnt : 0;
    if ( trace.enabled() )  trace.log( title + ".executeUpdate(): " + count );
    return( count );
} // executeUpdate


public int
executeUpdate( String sql, String columns[] )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".executeUpdate('" + sql + "'," + 
		   (columns == null ? "null" : "[" + columns.length + "]") + 
		   ")" );

    exec( sql, UPDATE );
    int count = ((rslt_items & RSLT_ROW_CNT) != 0) ? rslt_val_rowcnt : 0;
    if (trace.enabled()) trace.log(title + ".executeUpdate(): " + count);
    return( count );
} // executeUpdate


/*
** Name: execute
**
** Description:
**	Execute an SQL statement and return an indication as to the
**	type of the result.
**
**	Parameters related to auto generated keys are ignored.
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
**	21-Feb-03 (gordy)
**	    Added auto gen key methods.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Clean up tracing of array parameter.
*/

public boolean 
execute( String sql )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".execute( '" + sql + "' )" );
    exec( sql, UNKNOWN );
    boolean isRS = (resultSet != null);
    if ( trace.enabled() )  trace.log( title + ".execute(): " + isRS );
    return( isRS );
} // execute


public boolean 
execute( String sql, int autoGeneratedKeys )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".execute( '" + 
					sql + "'" + autoGeneratedKeys + ")" );
    switch( autoGeneratedKeys )
    {
    case RETURN_GENERATED_KEYS :    break;
    case NO_GENERATED_KEYS :	    break;
    default : throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    }

    exec( sql, UNKNOWN );
    boolean isRS = (resultSet != null);
    if ( trace.enabled() )  trace.log( title + ".execute(): " + isRS );
    return( isRS );
} // execute


public boolean 
execute( String sql, int columns[] )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".execute( '" + sql + "'," + 
		   (columns == null ? "null" : "[" + columns.length + "]") + 
		   ")" );

    exec( sql, UNKNOWN );
    boolean isRS = (resultSet != null);
    if ( trace.enabled() )  trace.log( title + ".execute(): " + isRS );
    return( isRS );
} // execute


public boolean 
execute( String sql, String columns[] )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".execute( '" + sql + "'," + 
		   (columns == null ? "null" : "[" + columns.length + "]") + 
		   ")" );

    exec( sql, UNKNOWN );
    boolean isRS = (resultSet != null);
    if ( trace.enabled() )  trace.log( title + ".execute(): " + isRS );
    return( isRS );
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
    if ( trace.enabled() )  trace.log(title + ".getResultSet(): " + resultSet);
    return( resultSet );
} // getResultSet

/*
** Name: getWarnings
**
** Description:
**      Retrieve the connection warning/message list.
**
** Input:
**      None.
**
** Output:
**      None.
**
** Returns:
**      SQLWarning  SQL warning/message list, may be NULL.
**
** History:
**       11-18-09 (rajus01) SD issue 140992, Bug 122977
**          Created.
*/

public SQLWarning
getWarnings()
    throws SQLException
{
    return( super.getWarnings() );
} // getWarnings



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
    int count = (resultSet != null  ||  (rslt_items & RSLT_ROW_CNT) == 0) 
		? -1 : rslt_val_rowcnt;
    if ( trace.enabled() )  trace.log( title + ".getUpdateCount(): " + count );
    return( count );
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
**	19-Feb-03 (gordy)
**	    Clear just the direct query results, not all results.
*/

public boolean
getMoreResults()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getMoreResults(): " + false );
    clearQueryResults( true );
    return( false );
} // getMoreResults


/*
** Name: getMoreResults
**
** Description:
**	Returns an indication that the next result is a result set.
**
**	We only have a single result value, so we just clear the current
**	results (current result-set handled as per app request) and 
**	return false.
**
** Input:
**	current	    How to handle current result-set.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if next result is a result set, false otherwise.
**
** History:
**	19-Feb-03 (gordy)
**	    Created.
*/

public boolean
getMoreResults( int current )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getMoreResults(" + current + "): " + false );
    
    switch( current )
    {
    case Statement.KEEP_CURRENT_RESULT :
	clearQueryResults( false );	// Don't close result-set
	break;

    case Statement.CLOSE_CURRENT_RESULT :
    case Statement.CLOSE_ALL_RESULTS :
	clearQueryResults( true );
	break;

    default :
	throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    }

    return( false );
} // getMoreResults


/*
** Name: getGeneratedKeys
**
** Description:
**	Retrieve table and/or object keys
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	ResultSet   Result-set containing table/object keys
**
** History:
**	21-Feb-03 (gordy)
**	    Created.
**	 3-Jul-03 (gordy)
**	    Set DBMS types to LOGKEY/TBLKEY.
**	26-Sep-03 (gordy)
**	    Result-set data now stored as SqlByte SqlData objects.
*/

public synchronized ResultSet
getGeneratedKeys()
    throws SQLException
{
    ResultSet	rs = rsEmpty;
    SqlData	data[][];
    SqlByte	bytes;
    
    if ( (rslt_items & RSLT_TBLKEY) != 0 )
	if ( (rslt_items & RSLT_OBJKEY) != 0 )
	{
	    if ( rsmdKeys == null )
	    {
		rsmdKeys = new JdbcRSMD( (short)2, trace );
		rsmdKeys.setColumnInfo( "table_key", 1, (int)Types.BINARY, 
					(short)DBMS_TYPE_TBLKEY, 
					(short)MSG_RPV_TBLKEY_LEN, 
					(byte)0, (byte)0, false );
		rsmdKeys.setColumnInfo( "object_key", 2, (int)Types.BINARY, 
					(short)DBMS_TYPE_LOGKEY, 
					(short)MSG_RPV_OBJKEY_LEN, 
					(byte)0, (byte)0, false );
	    }

	    data = new SqlData[1][];
	    data[0] = new SqlData[2];
	    
	    bytes = new SqlByte( MSG_RPV_TBLKEY_LEN );
	    bytes.put( rslt_val_tblkey );
	    data[0][0] = bytes;
	    
	    bytes = new SqlByte( MSG_RPV_OBJKEY_LEN );
	    bytes.put( rslt_val_objkey );
	    data[0][1] = bytes;
	    
	    rs = new RsltData( conn, rsmdKeys, data );
	}
	else
	{
	    if ( rsmdTblKey == null )
	    {
		rsmdTblKey = new JdbcRSMD( (short)1, trace );
		rsmdTblKey.setColumnInfo( "table_key", 1, (int)Types.BINARY, 
					  (short)DBMS_TYPE_TBLKEY, 
					  (short)MSG_RPV_TBLKEY_LEN, 
					  (byte)0, (byte)0, false );
	    }

	    data = new SqlData[1][];
	    data[0] = new SqlData[1];
	    
	    bytes = new SqlByte( MSG_RPV_TBLKEY_LEN );
	    bytes.put( rslt_val_tblkey );
	    data[0][0] = bytes;
	    
	    rs = new RsltData( conn, rsmdTblKey, data );
	}
    else  if ( (rslt_items & RSLT_OBJKEY) != 0 )
    {
	if ( rsmdObjKey == null )
	{
	    rsmdObjKey = new JdbcRSMD( (short)1, trace );
	    rsmdObjKey.setColumnInfo( "object_key", 1, (int)Types.BINARY, 
				      (short)DBMS_TYPE_LOGKEY, 
				      (short)MSG_RPV_OBJKEY_LEN, 
				      (byte)0, (byte)0, false );
	}

	data = new SqlData[1][];
	data[0] = new SqlData[1];
	    
	bytes = new SqlByte( MSG_RPV_OBJKEY_LEN );
	bytes.put( rslt_val_objkey );
	data[0][0] = bytes;
	    
	rs = new RsltData( conn, rsmdObjKey, data );
    }
    else  if ( rsEmpty == null )
    {
	if ( rsmdEmpty == null )
	{
	    rsmdEmpty = new JdbcRSMD( (short)1, trace );
	    rsmdEmpty.setColumnInfo( "no_key", 1, (int)Types.BINARY, 
				     (short)DBMS_TYPE_TBLKEY, 
				     (short)MSG_RPV_TBLKEY_LEN, 
				     (byte)0, (byte)0, false );
	}

	rs = rsEmpty = new RsltData( conn, rsmdEmpty, null );
    }

    if ( trace.enabled() )  trace.log( title + ".getGeneratedKeys: " + rs );
    return( rs );
} // getGeneratedKeys


/*
** Name: clearResults
**
** Description:
**	Clear all query results: both direct results and auxilliary results.
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
**	19-Feb-03 (gordy)
**	    Separate direct results from general results.
*/

protected void
clearResults()
{
    clearQueryResults( true );
    super.clearResults();
    return;
} // clearResults


/*
** Name: clearQueryResults
**
** Description:
**	Clears the direct results of a query: update count and result-set.
**	Clearing the result-set is optional and controlled by close parameter.
**
** Input:
**	close	Close result-set?
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

protected synchronized void
clearQueryResults( boolean close )
{
    if ( close  &&  resultSet != null )
    {
	try { resultSet.shut(); }
	catch( SQLException ignore ) {}
	resultSet = null;
    }

    rslt_items &= ~RSLT_ROW_CNT;
    rslt_val_rowcnt = 0;
    return;
} // clearQueryResults


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
**	31-Oct-02 (gordy)
**	    Batch list allocation now done with newBatch().
*/

public void
addBatch( String sql )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".addBatch( '" + sql + "' )" );
    if ( batch == null )  newBatch();
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
**	31-Oct-02 (gordy)
**	    Check for NULL batch instead of allocating.
*/

public void
clearBatch()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".clearBatch()" );
    if ( batch != null )  synchronized( batch ) { batch.clear(); }
    return;
} // clearBatch


/*
** Name: executeBatch
**
** Description:
**	Execute the queries in the batch set.
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
**	int []	    Array of update counts for each query.
**
** History:
**	25-Mar-10 (gordy)
**	    Implement support for actual batch processing.  Simulated
**	    batch processing moved to iterateBatch().
*/

public int []
executeBatch()
    throws SQLException
{
    int results[];

    if ( trace.enabled() )  trace.log( title + ".executeBatch()" );
    if ( batch == null )  return( BatchExec.noResults );

    /*
    ** Execute each query individually when batch processing
    ** isn't supported on the connection or has been disabled.
    */
    if ( (conn.cnf_flags & conn.CNF_BATCH) == 0  ||
	 conn.db_protocol_level < DBMS_API_PROTO_6 ) 
	results = iterateBatch();
    else
	synchronized( batch )
	{
	    BatchExec batchEx = new BatchExec( conn );
	    try { results = batchEx.execute( batch, parse_escapes ); }
	    finally { batch.clear(); }
	}

    return( results );
} // executeBatch


/*
** Name: iterateBatch
**
** Description:
**	Individually execute the queries in the batch set.
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
**	31-Oct-02 (gordy)
**	    Check for NULL batch instead of allocating.
**	19-Feb-03 (gordy)
**	    Use SUCCESS_NO_INFO result rather than integer literal.
**	25-Mar-10 (gordy)
**	    Extracted from executeBatch() as alternative to actual
**	    batch processing.
*/

private int []
iterateBatch()
    throws SQLException
{
    int results[];

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
		String sql = (String)batch.pollFirst();
		if ( trace.enabled() )  
		    trace.log( title + ".executeBatch[" + i + "]: '" 
				     + sql + "'" );

		exec( sql, UPDATE );
		results[ i ] = ((rslt_items & RSLT_ROW_CNT) != 0) 
			       ? rslt_val_rowcnt : Statement.SUCCESS_NO_INFO;

		if ( trace.enabled() )  
		    trace.log( title + ".executeBatch[" + i + "] = " 
				     + results[ i ] );
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
} // iterateBatch


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
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	19-Feb-03 (gordy)
**	    Added method to calculate fetch size.
**	15-Apr-03 (gordy)
**	    Added connection timezones separate from date formatters.
**	 7-Jul-03 (gordy)
**	    Added Ingres timezone config which affects date/time literals.
**	 4-Aug-03 (gordy)
**	    Created neew result-set class for updatable cursors.
**	20-Aug-03 (gordy)
**	    Send AUTO_CLOSE flag for query statements.
**	12-Sep-03 (gordy)
**	    Timezone replaced by GMT indicator.
**	 6-Oct-03 (gordy)
**	    Fetch first block of rows for select-loops and read-only cursors.
**	 1-Nov-03 (gordy)
**	    Implemented updatable result-sets.
**	19-Jun-06 (gordy)
**	    I give up... GMT indicator replaced with connection.
**	 9-Nov-06 (gordy)
**	    Added writeQueryText() to segment query text.
**	15-Nov-06 (gordy)
**	    Request LOB Locators when opening a cursor.  Locators
**	    are not utilized for non-selects or with select loops
**	    (due to tuple stream conflict with locator requests).
**	30-Jan-07 (gordy)
**	    Check if locators are enabled.
**	26-Feb-07 (gordy)
**	    Added configuration flags for locators in select loops.
**	 6-Apr-07 (gordy)
**	    Support scrollable cursors.
**	 3-Mar-09 (gordy)
**	    Check that scrollable cursors are enabled.
*/

private void
exec( String text, int mode )
    throws SQLException
{
    SqlParse	parser = new SqlParse( text, conn );
    int		type = parser.getQueryType();
    JdbcRSMD	rsmd;
    String	cursor = null;

    clearResults();
    
    if ( mode == UNKNOWN )  
	mode = (type == SqlParse.QT_SELECT) ? QUERY : UPDATE;
    
    if ( type == SqlParse.QT_DELETE  ||  type == SqlParse.QT_UPDATE )
	cursor = parser.getCursorName();

    msg.lock();
    try
    {	
	String	sql = parser.parseSQL( parse_escapes );

	if ( mode == QUERY )
	{
	    int	concurrency = getConcurrency( parser.getConcurrency() );

	    /*
	    ** Execute a select loop retrieval if the following
	    ** conditions exist:
	    **
	    ** 1) Select loops are enabled.
	    ** 2) No cursor name has been assigned.
	    ** 3) Result-set is not scrollable.
	    ** 4) Result-set is not updatable.
	    ** 5) Protocol supports select loops.
	    */
	    if ( 
	         conn.select_loops  &&  crsr_name == null  &&  
		 rs_type == ResultSet.TYPE_FORWARD_ONLY  &&
		 concurrency != DRV_CRSR_UPDATE  &&
		 conn.msg_protocol_level >= MSG_PROTO_2
	       )
	    {
		/*
		** Select statement using a select loop.
		*/
		boolean needEOG = true;
		msg.begin( MSG_QUERY );
		msg.write( MSG_QRY_EXQ );
		
		if ( conn.msg_protocol_level >= MSG_PROTO_3 )
		{
		    short flags = MSG_QF_AUTO_CLOSE;

		    if ( enableLocators( true ) )  flags |= MSG_QF_LOCATORS;

		    if ( rs_fetch_size != 1 )  
		    {
			flags |= MSG_QF_FETCH_FIRST;
			needEOG = false;
		    }

		    msg.write( MSG_QP_FLAGS );
		    msg.write( (short)2 );
		    msg.write( flags );
		}
		
		writeQueryText( sql );
		msg.done( true );

		if ( (rsmd = readResults( timeout, needEOG )) == null )
		    throw SqlExFactory.get( ERR_GC4017_NO_RESULT_SET );

		resultSet = new RsltSlct( conn, this, rsmd, rslt_val_stmt, 
					getPreFetchSize(), msg.moreMessages());
	    }
	    else
	    {
		/*
		** Select statement using a cursor.  Generate a 
		** cursor ID if not provided by the application.
		*/
		short	flags = 0;
		boolean	needEOG = true;
		cursor = (crsr_name != null) ? crsr_name 
					     : conn.getUniqueID( crsr_prefix );


		/*
		** Parser removes FOR READONLY clause because it isn't
		** a part of Ingres SELECT syntax.  Tell server that
		** cursor should be READONLY (kludge older protocol
		** by restoring clause to query).
		*/
		if ( concurrency == DRV_CRSR_READONLY )
		    if ( conn.msg_protocol_level < MSG_PROTO_2 )
			sql += " for readonly";
		    else  
		    {
			flags |= MSG_QF_READONLY;

			/*
			** Pre-fetch first block of rows, but not
			** for scrollable cursors since initial
			** operation may not involve first rows.
			*/
			if ( 
			     conn.msg_protocol_level >= MSG_PROTO_3  &&
			     rs_type == ResultSet.TYPE_FORWARD_ONLY  &&
			     rs_fetch_size != 1 
			   )
			{
			    flags |= MSG_QF_FETCH_FIRST;
			    needEOG = false;
			}
		    }

		/*
		** Don't auto close scrollable cursors since the
		** application may request a scroll operation 
		** after reaching the end of the result-set.
		*/
		if ( (conn.cnf_flags & conn.CNF_CURS_SCROLL) != 0  &&
		     rs_type != ResultSet.TYPE_FORWARD_ONLY )
		    flags |= MSG_QF_SCROLL;
		else  if ( conn.msg_protocol_level >= MSG_PROTO_3 )
		    flags |= MSG_QF_AUTO_CLOSE;

		if ( enableLocators( false ) )  
		    flags |= MSG_QF_LOCATORS;

		msg.begin( MSG_QUERY );
		msg.write( MSG_QRY_OCQ );
		
		if ( flags != 0 )
		{
		    msg.write( MSG_QP_FLAGS );
		    msg.write( (short)2 );
		    msg.write( flags );
		}
		
		msg.write( MSG_QP_CRSR_NAME );
		msg.write( cursor );
		writeQueryText( sql );
		msg.done( true );

		if ( (rsmd = readResults( timeout, needEOG )) == null )
		    throw SqlExFactory.get( ERR_GC4017_NO_RESULT_SET );

		switch( rslt_flags & (MSG_RF_SCROLL | MSG_RF_READ_ONLY) )
		{
		case 0 :		// Updatable
		    if ( rs_type != ResultSet.TYPE_FORWARD_ONLY  ||
			 rs_concur != DRV_CRSR_UPDATE )
			warnings = SqlWarn.get( ERR_GC4016_RS_CHANGED );
		    
		    /*
		    ** The cursor name is passed to the result-set 
		    ** for updatable cursors (JDBC 2.1 API spec).
		    */
		    resultSet = new RsltUpd( conn, this, rsmd, rslt_val_stmt, 
					     cursor, parser.getTableName() );
		    if ( msg.moreMessages() )  readResults( timeout, true );
		    break;

		case MSG_RF_SCROLL :	// Scrollable, updatable
		    if ( rs_type != ResultSet.TYPE_SCROLL_SENSITIVE  ||
			 rs_concur != DRV_CRSR_UPDATE )
			warnings = SqlWarn.get( ERR_GC4016_RS_CHANGED );
		    
		    /*
		    ** The cursor name is passed to the result-set 
		    ** for updatable cursors (JDBC 2.1 API spec).
		    */
		    resultSet = new RsltScrUpd( conn, this, rsmd, 
						rslt_val_stmt, cursor, 
						parser.getTableName() );
		    if ( msg.moreMessages() )  readResults( timeout, true );
		    break;

		case MSG_RF_READ_ONLY :
		    if ( rs_type != ResultSet.TYPE_FORWARD_ONLY  ||
			 rs_concur == DRV_CRSR_UPDATE )
			warnings = SqlWarn.get( ERR_GC4016_RS_CHANGED );
		    
		    /*
		    ** The cursor name is passed to the result-set if
		    ** provided by the application (JDBC 2.1 API spec).
		    */
		    resultSet = new RsltCurs( conn, this, rsmd, rslt_val_stmt, 
					      crsr_name, getPreFetchSize(),
					      msg.moreMessages() );
		    break;

		case MSG_RF_SCROLL | MSG_RF_READ_ONLY :	
		    /*
		    ** Readonly scrollable cursors are currently
		    ** always insensitive to changes.
		    */
		    if ( rs_type != ResultSet.TYPE_SCROLL_INSENSITIVE  ||
			 rs_concur == DRV_CRSR_UPDATE )
			warnings = SqlWarn.get( ERR_GC4016_RS_CHANGED );
		    
		    /*
		    ** The cursor name is passed to the result-set if
		    ** provided by the application (JDBC 2.1 API spec).
		    */
		    resultSet = new RsltScroll( conn, this, rsmd, rslt_val_stmt,
						crsr_name, getPreFetchSize(),
						msg.moreMessages() );
		    break;
		}

		msg.unlock(); 
	    }
	}
	else  if ( cursor != null )
	{
	    /*
	    ** Positioned Delete/Update.
	    */
	    msg.begin( MSG_QUERY );
	    msg.write( (type == SqlParse.QT_DELETE) ? MSG_QRY_CDEL 
						    : MSG_QRY_CUPD );
	    msg.write( MSG_QP_CRSR_NAME );
	    msg.write( cursor );
	    writeQueryText( sql );
	    msg.done( true );

	    if ( readResults( timeout, true ) != null )	// shouldn't happen
		throw SqlExFactory.get( ERR_GC4018_RESULT_SET_NOT_PERMITTED );
	    msg.unlock(); 
	}
	else
	{
	    /*
	    ** Non-query statement.
	    */
	    msg.begin( MSG_QUERY );
	    msg.write( MSG_QRY_EXQ );
	    writeQueryText( sql );
	    msg.done( true );

	    if ( (rsmd = readResults( timeout, true )) == null )
		msg.unlock();		// We be done.
	    else
	    {
		/*
		** Query unexpectedly returned a result-set.
		** We need to cleanup the tuple stream and
		** statement in the JDBC server.  The easiest
		** way to do this is go ahead and create a
		** result-set and close it.
		*/
		resultSet = new RsltSlct( conn, this, rsmd, 
					  rslt_val_stmt, 1, false );
		try { resultSet.shut(); }
		catch( SQLException ignore ) {}
		resultSet = null;
		throw SqlExFactory.get( ERR_GC4018_RESULT_SET_NOT_PERMITTED );
	    }
	}
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".execute(): error executing query" );
	if ( trace.enabled( 1 ) )  SqlExFactory.trace( ex, trace );
	msg.unlock(); 
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
    msg.cancel();
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
    closed = true;
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
	trace.log( title + ".getConnection(): " + conn.jdbc );
    return( conn.jdbc );
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
    int	concurrency;
    
    if ( rs_concur == DRV_CRSR_UPDATE )  
	concurrency = ResultSet.CONCUR_UPDATABLE;
    else
	concurrency = ResultSet.CONCUR_READ_ONLY;   // JDBC default

    if ( trace.enabled() )  
	trace.log( title + ".getResultSetConcurrency(): " + concurrency );
    return( concurrency );
} // getResultSetConcurrency


/*
** Name: getResultSetHoldability
**
** Description:
**	Retrieves the default ResultSet holdability.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Default ResultSet holdability.
**
** History:
**	19-Feb-03 (gordy)
**	    Created.
*/

public int
getResultSetHoldability()
{
    if ( trace.enabled() )  
	trace.log( title + ".getResultSetHoldability(): " + rs_hold );
    return( rs_hold );
} // getResultSetHoldability


/*
** Name: isClosed
**
** Description:
**      Retrieves whether this Statement object has been closed.
**
** Input:
**      None.
**
** Output:
**      None.
**
** Returns:
**      boolean   True if this Statement object is closed; false otherwise.
**
** History:
**       6-May-09 (rajus01) SIR 121238
**          Implemented.
*/
public boolean 
isClosed()
throws SQLException
{
    if ( trace.enabled() )  trace.log(title + ".isClosed(): " + closed );
    return( closed );
} // isClosed

// The following JDBC 4.0 methods are not implemented at this time.

public void 
setPoolable(boolean poolable)
throws SQLException
{
    if ( trace.enabled() )  trace.log(title + ".setPoolable()");
    return;
} //setPoolable

public boolean isPoolable()
throws SQLException
{
    if ( trace.enabled() )  trace.log(title + ".isPoolable()");
    return false;
} //isPoolable

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
isWrapperFor(Class<?> iface)
throws SQLException
{
    if ( trace.enabled() )  trace.log(title + ".isWrapperFor(" + iface + ")");
    /*
    ** This approach seems to work for classes that actually don't
    ** wrap anything.
    */
    if( iface != null )
	return( iface.isInstance( this ) );

    throw SqlExFactory.get(ERR_GC4010_PARAM_VALUE);

} //isWrapperFor

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
unwrap(Class<T> iface)
throws SQLException
{
    if ( trace.enabled() )  trace.log(title + ".unwrap(" + iface + ")");
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
} //unwrap

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
	throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    }

    rs_fetch_dir = dir;
    return;
} // setFetchDirection


/*
** Name: getFetchSize
**
** Description:
**	Retrieves the suggested ResultSet pre-fetch size.  A value 
**	of 0 is returned if the fetch size has not been set.
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
**	rows	Pre-fetch size.
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
	throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );

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
    if ( max < 0 )  throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
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
    if ( max < 0 )  throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
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
    if ( seconds < 0 )  throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    timeout = seconds;
    return;
} // setQueryTimeout

/*
** Name: newBatch
**
** Description:
**	Provides synchronization for allocating a batch list.
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
**	31-Oct-02 (gordy)
**	    Extracted from addBatch().
*/

protected synchronized void
newBatch()
{
    /* Allocate only once. */
    if ( batch == null )  batch = new LinkedList();
    return;
} // newBatch


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
    if ( concurrency != DRV_CRSR_DBMS )  return( concurrency );
    if ( rs_concur != DRV_CRSR_DBMS )  return( rs_concur );
    return( conn.readOnly ? DRV_CRSR_READONLY : conn.cursor_mode );
} // getConcurrency


/*
** Name: getPreFetchSize
**
** Description:
**	Returns the cursor pre-fetch row limit for the current
**	result-set based on server suggested fetch size.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	    Fetch size.
**
** History:
**	28-Feb-03 (gordy)
**	    Created.
**	 4-Aug-03 (gordy)
**	    Removed readonly dependecy and calculate size soley on
**	    application and server settings (pre-fetch restrictions
**	    must be handled by caller).  Renamed due to conflict
**	    with interface method.
**	20-Jul-07 (gordy)
**	    Return indication that pre-fetching is disallowed or
**	    unspecified.  Result-sets now retrieve application
**	    suggested fetch size directly from this object.
*/

protected int
getPreFetchSize()
{
    /*
    ** Pre-fetching is only allowed if the server returned
    ** a suggested pre-fetch size, otherwise force single row.  
    */
    return( ((rslt_items & RSLT_PREFETCH) == 0) ? -1 :
	    ((rslt_val_fetch < 1) ? 0 : rslt_val_fetch) );
} // getPreFetchSize


/*
** Name: enableLocators
**
** Description:
**	Returns TRUE if LOB Locators are enabled for the
**	current connection state.
**
** Input:
**	select_loop	Is query a select loop.
**
** Output:
**	None.
**
** Returns:
**	boolean		TRUE if LOB Locators enabled.
**
** History:
**	26-Feb-07 (gordy)
**	    Created.
*/

protected boolean
enableLocators( boolean select_loop )
{
    /*
    ** Build the set of configuration flags applicable to
    ** the current connection state: general usage, auto-
    ** commit active, select_loop retrieval.
    */
    int loc_flags = conn.CNF_LOCATORS;
    if ( conn.autoCommit )  loc_flags |= conn.CNF_LOC_AUTO;
    if ( select_loop )  loc_flags |= conn.CNF_LOC_LOOP;

    /*
    ** The configured enable flags must match those for
    ** the current connection state.
    */
    return( (conn.cnf_flags & loc_flags) == loc_flags );
} // enableLocators


/*
** Name: readResults
**
** Description:
**	Read server result messages with optional timeout.
**	Returns when end-of-group is reached, or optionally
**	when a RESULT message is received.  See super-class 
**	readResult() method for details on message processing.
**
** Input:
**	timeout		Time in seconds (negative for milli-seconds).
**	needEOG		Read messages until EOG.
**
** Output:
**	None.
**
** Returns:
**	JdbcRSMD	Result-set Meta-data from DESC message.
**
** History:
**	31-Oct-02 (gordy)
**	    Created.
**	 6-Oct-03 (gordy)
**	    Added needEOG parameter to allow RESULT message interrupts.
*/

protected JdbcRSMD
readResults( int timeout, boolean needEOG )
    throws SQLException
{
    JdbcRSMD rsmd;
    enableTimer( timeout );

    try { rsmd = readResults( needEOG ); }
    finally
    {
	disableTimer();
	timed_out = false;
    }

    return( rsmd );
} // readResults


/*
** Name: readResults
**
** Description:
**	Read server result messages.  Returns when end-of-group is 
**	reached, or optionally when a RESULT message is received.  
**	See super-class readResult() method for details on message 
**	processing.
**
** Input:
**	needEOG		Read messages until EOG.
**
** Output:
**	None.
**
** Returns:
**	JdbcRSMD	Result-set Meta-data from DESC message.
**
** History:
**	 6-Oct-03 (gordy)
**	    Created.
*/

protected JdbcRSMD
readResults( boolean needEOG )
    throws SQLException
{
    JdbcRSMD rsmd = null;
    
    /*
    ** Read results until interrupted or end-of-group.
    */
    do
    {
        /*
        ** There should only be one descriptor,
        ** but just to be safe, return last.
        */
        JdbcRSMD current = readResults(); 
        if ( current != null )  rsmd = current;
    } while( needEOG  &&  msg.moreMessages() );

    return( rsmd );
} // readResults


/*
** Name: readDesc
**
** Description:
**	Read a query result DESC message.  Overrides the default method 
**	in DrvObj since descriptors are expected.  Disable query timeouts.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	JdbcRSMD    Query result data descriptor.
**
** History:
**	 8-Sep-99 (gordy)
**	    Created.
*/

protected JdbcRSMD
readDesc()
    throws SQLException
{
    disableTimer();
    return( JdbcRSMD.load( conn ) );
} // readDesc


/*
** Name: readDesc
**
** Description:
**	Read a query result DESC message.  Overrides the default method 
**	in DrvObj since descriptors are expected.  Disable query timeouts.
**	Will reload an existing RSMD or allocate new RSMD if NULL.
**
** Input:
**	rsmd		ResultSet meta-data to load, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	JdbcRSMD	Query result data descriptor.
**
** History:
**	 8-Sep-99 (gordy)
**	    Created.
*/

protected JdbcRSMD
readDesc( JdbcRSMD rsmd )
    throws SQLException
{
    disableTimer();
    
    if ( rsmd == null )
	rsmd = JdbcRSMD.load( conn );
    else
	rsmd.reload( conn );
    
    return( rsmd );
} // readDesc


/*
** Name: readError
**
** Description:
**	Read a query result ERROR message.  Overrides the default method
**	in DrvObj to disable query timeouts.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	SQLException	Error or NULL.
**
** History:
**	31-Oct-02 (gordy)
**	    Created.
*/

protected SQLException
readError()
    throws SQLException
{
    SQLException   ex;
    
    disableTimer();
    ex = super.readError();

    /*
    ** We translate the query canceled error from a timeout 
    ** into a JDBC driver error.
    */
    if ( timed_out  &&  ex != null  &&
	 ex.getErrorCode() == E_AP0009_QUERY_CANCELLED ) 
	ex = SqlExFactory.get( ERR_GC4006_TIMEOUT );

    return( ex );
} // readError


/*
** Name: readResult
**
** Description:
**	Read a query result RESULT message.  Overrides the default method
**	in DrvObj to disable query timeouts and interrupt processing.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	True to interrupt message processing.
**
** History:
**	31-Oct-02 (gordy)
**	    Created.
**	 6-Oct-03 (gordy)
**	    Renamed per changes in super-class.  Always interrupt.
*/

protected boolean
readResult()
    throws SQLException
{
    disableTimer();
    super.readResult();
    return( true );
} // readResult


/*
** Name: enableTimer
**
** Description:
**	Sets a timer for a requested timeout.  When the time elapses, 
**	timed_out will be set TRUE.  Timer may be disabled prior to 
**	timeout by calling disableTimer.
**
**	A negative timeout values is interpreted as milli-seconds while
**	a positive value is assumed to be seconds.  A zero value will
**	disable the timer.
**
** Input:
**	timeout	Timeout.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	31-Oct-02 (gordy)
**	    Created.
*/

private void
enableTimer( int timeout )
{
    timed_out = false;

    if ( timeout == 0 )
	timer = null;
    else
    {
	timer = new Timer( timeout, (Timer.Callback)this );
	timer.start();
    }
    return;
} // enableTimer


/*
** Name: timeExpired
**
** Description:
**	Implements the callback method for the Timer Callback 
**	interface.  Called by the Timer object created in 
**	readResults().  Cancel the current query.
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
**	31-Oct-02 (gordy)
**	    Created.
*/

public void
timeExpired()
{
    if ( timer != null )  
    {
	timed_out = true;
	timer = null;
    }
    try { msg.cancel(); }
    catch( Exception ignore ) {}
    return;
} // timeExpired


/*
** Name: disableTimer
**
** Description:
**	Disable a timer started by enableTimer().
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
**	31-Oct-02 (gordy)
**	    Created.
*/

private void
disableTimer()
{
    if ( timer != null )  
    {
	timer.interrupt();
	timer = null;
    }
    return;
} // disableTimer


/*
** Name: BatchExec
**
** Description:
**	Class which implements batch query execution.
**
**  Public Data:
**
**	noResults		Empty batch query results.
**
**  Public Methods:
**
**	execute			Execute batch.
**
**  Private Data:
**
**	resultCount		Number of batch query results.
**	batchExList		Errors from batch queries.
**	exList			Temp error accumulation.
**
**  Private Methods:
**
**	batchResults		Returns batch query results.
**	readResults		Read batch results.
**	readError		Read error message.
**	readResult		Read result message.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

protected static class
BatchExec
    extends DrvObj
{

    public static final int	noResults[] = new int[0];

    private int			queryResults[] = noResults;
    private int			resultCount = 0;
    private SQLException	batchExList = null;
    private SQLException	exList = null;


/*
** Name: BatchExec
**
** Description:
**	Class constructor.
**
** Input:
**	conn		Database connection.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

public
BatchExec( DrvConn conn )
{
    super( conn );

    title = trace.getTraceName() + "-BatchExec[" + msg.connID() + "]";
    tr_id = "Batch[" + msg.connID() + "]";

} // BatchExec


/*
** Name: execute
**
** Description:
**	Execute queries in a batch set.
**
** Input:
**	batch		List of queries to be executed.
**	parse_escapes	Parse SQL escapce sequences?
**
** Output:
**	None.
**
** Returns:
**	int []	Batch query results.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

public int []
execute( LinkedList batch, boolean parse_escapes )
    throws SQLException
{
    String	sql;
    int		count = 0;

    if ( batch == null  ||  batch.peekFirst() == null )  return( noResults );

    clearResults();
    msg.lock();

    try
    {	
	/*
	** Send each query in batch to server.
	*/
	while( (sql = (String)batch.pollFirst()) != null )
	{
	    if ( trace.enabled() )  
		trace.log( title + ".executeBatch[" + count + "]: '" 
				 + sql + "'" );

	    sql = (new SqlParse( sql, conn )).parseSQL( parse_escapes );

	    count++;
	    msg.begin( MSG_BATCH );
	    msg.write( MSG_BAT_EXQ );
	    writeQueryText( sql );

	    /*
	    ** All but last query in batch are marked EOB.
	    ** Last query marked EOG.
	    */
	    msg.done( (batch.peekFirst() == null) ? MSG_HDR_EOG 
						  : MSG_HDR_EOB );
	}

	/*
	** Read batch results returned by server.
	*/
	readResults( count );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".execute(): error executing batch" );
	if ( trace.enabled( 1 ) )  SqlExFactory.trace( ex, trace );
	throw ex;
    }
    finally
    {
	msg.unlock(); 
    }

    return( batchResults() );
} // execute


/*
** Name: batchResults
**
** Description:
**	Assemble the batch query execution results.  If any
**	batch query errors occured, the errors and batch
**	results are combined into a BatchUpdateException
**	and throws.  Otherwise, the batch query results are
**	returned.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int []	Batch query results.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

protected int []
batchResults()
    throws SQLException
{
    int results[];

    /*
    ** Space was provided to hold results for all queries
    ** which were executed.  Critical errors could have
    ** resulted in some queries not being executed. Limit
    ** results to those returned by server.
    */
    if ( resultCount <= 0 )  
	results = noResults;
    else  if ( resultCount >= queryResults.length )
	results = queryResults;
    else
    {
	results = new int[ resultCount ];
	System.arraycopy( queryResults, 0, results, 0, resultCount );
    }

    /*
    ** Batch query errors are accumulated during result processing.
    */
    if ( batchExList != null )
    {
	BatchUpdateException bue;

	/*
	** Generate a batch exception from the exceptions 
	** accumulated during result processing and the 
	** batch results.
	*/
	bue = new BatchUpdateException( batchExList.getMessage(), 
					batchExList.getSQLState(),
					batchExList.getErrorCode(), results );

	bue.setNextException( batchExList.getNextException() );
	throw bue;
    }

    return( results );
} // batchResults


/*
** Name: readResults
**
** Description:
**	Cover for super-class method.  Initialize batch results.
**
** Input:
**	batchCount	Number of batch queries.
**
** Output:
**	None.
**
** Returns:
**	JdbcRSMD	Result-set meta-data.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

protected JdbcRSMD
readResults( int batchCount )
    throws SQLException, XaEx
{
    /*
    ** Initialize batch results.
    */
    if ( batchCount > 0 )  queryResults = new int[ batchCount ];
    if ( resultCount < queryResults.length )  
	queryResults[ resultCount ] = Statement.SUCCESS_NO_INFO;

    JdbcRSMD rsmd = super.readResults();

    /*
    ** Errors are first accumulated on exList and moved to
    ** batchExList when a batch query result is received.
    ** Batch query exceptions are not thrown at this level.
    ** Any exceptions remaining on exList are general
    ** processing errors unrelated to a specific batch
    ** query and need to be thrown.
    */
    if ( exList != null )  throw exList;

    return( rsmd );
} // readResults


/*
** Name: readError
**
** Description:
**	Cover for super-class method.  Errors are accumulated
**	on temp list for later processing rather than passing
**	back to be thrown at end of results.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	SQLException	Error or NULL.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

protected SQLException
readError()
    throws SQLException
{
    SQLException ex = super.readError();

    /*
    ** Only interested in SQLExceptions
    */
    if ( ex == null  ||  ex instanceof XaEx )  return( ex );

    /*
    ** Save exception for later processing.
    */
    if ( exList == null )
	exList = ex;
    else
	exList.setNextException( ex );

    return( null );
} // readError


/*
** Name: readResult
**
** Description:
**	Cover for super-class method.  Batch query results
**	are collected in batchResults.  Errors accumulated
**	previously in readError() are saved on batchExList.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean		True for interrupt, False to continue.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

protected boolean
readResult()
    throws SQLException
{
    boolean interrupt = super.readResult();

    /*
    ** A RESULT message is returned for each batch query executed.  
    ** Batch query results are marked end-of-batch.  Query results 
    ** are followed by a terminating (non-batch) RESULT message.
    */
    if ( ! msg.isEndOfBatch() )
    {
	/*
	** A non-batch result should terminate response.
	*/
	if ( msg.moreMessages() )
	{
	    if ( trace.enabled( 3 ) )
		trace.write( tr_id + ": unexpected RESULT message (not EOG)" );
	}
	
	return( interrupt );
    }

    /*
    ** There should be at most one batch result for each query submitted.
    */
    if ( resultCount >= queryResults.length )
    {
	/*
	** We've already saved all the batch query results.  Any
	** (unexpected) additional results are treated the same
	** as the (expected) terminating RESULT message.  Note
	** case where terminating RESULT message is also marked
	** as a betch result.
	*/
	if ( trace.enabled( 3 ) )
	    if ( ! msg.moreMessages() )
		trace.write( tr_id + ": terminating result marked as batch" );
	    else
		trace.write( tr_id + ": additional batch result received" );

	return( interrupt );
    }

    /*
    ** Errors received prior to a batch result are accumulated
    ** in the batch exception list and current batch entry is 
    ** marked as failed.
    */
    if ( exList != null )
    {
	if ( batchExList == null )
	    batchExList = exList;
	else
	    batchExList.setNextException( exList );

	exList = null;
	queryResults[ resultCount ] = Statement.EXECUTE_FAILED;
    }

    /*
    ** The query result is always retrieved to ensure it is 
    ** cleared for the next query.  The result is only saved 
    ** if current batch entry has not been marked as failed.
    */
    int result = getQueryResult();

    if ( queryResults[ resultCount ] == Statement.SUCCESS_NO_INFO )
	queryResults[ resultCount ] = result;

    if ( trace.enabled() )  
	switch( queryResults[ resultCount ] )
	{
	case Statement.SUCCESS_NO_INFO :
	    trace.log( title + ".executeBatch[" + resultCount + "] = " 
			     + "SUCCESS_NO_INFO" );
	    break;

	case Statement.EXECUTE_FAILED :
	    trace.log( title + ".executeBatch[" + resultCount + "] = " 
			     + "EXECUTE_FAILED" );
	    break;

	default :
	    trace.log( title + ".executeBatch[" + resultCount + "] = " 
			     + queryResults[ resultCount ] );
	    break;
	}

    /*
    ** Initialize next batch result (if any).
    */
    if ( ++resultCount < queryResults.length )
	queryResults[ resultCount ] = Statement.SUCCESS_NO_INFO;

    /*
    ** Batch results should not terminate response.
    */
    if ( ! msg.moreMessages() )
    {
	if ( trace.enabled( 3 ) )
	    trace.write( tr_id + ": batch result terminates response" );
    }

    return( interrupt );
} // readResult()


/*
** Name: getQueryResult
**
** Description:
**	Returns an integer result value extracted from the current
**	batch query set of result values.  If no suitable query
**	result is available, SUCCESS_NO_INFO is returned.
**
**	By default, only the row count is returned.  Sub-classes
**	should override this method if some other query results
**	should be returned.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Batch query result.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

protected int
getQueryResult()
{
    /*
    ** The only batch query result returned by default is the row count.
    */
    if ( (rslt_items & RSLT_ROW_CNT) != 0 )
    {
	/*
	** Consume and return the result value.
	*/
	rslt_items &= ~RSLT_ROW_CNT;
	return( rslt_val_rowcnt );
    }

    return( Statement.SUCCESS_NO_INFO );
} // getQueryResult()


} // class BatchExec


} // class JdbcStmt

