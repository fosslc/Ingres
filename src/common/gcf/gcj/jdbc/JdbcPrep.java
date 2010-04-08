/*
** Copyright (c) 1999, 2010 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: JdbcPrep.java
**
** Description:
**	Defines class which implements the JDBC PreparedStatement interface.
**
**  Classes:
**
**	JdbcPrep	Implements the JDBC PreparedStatement interface.
**	BatchExec	Batch query execution.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	 8-Sep-99 (gordy)
**	    Created new base class for class which interact with
**	    the JDBC server and extracted common data and methods.
**	    Synchronize entire request/response with JDBC server.
**	13-Sep-99 (gordy)
**	    Implemented error code support.
**	29-Sep-99 (gordy)
**	    Added classes for BLOB streams and implemented support
**	    for BLOBs.  Use DbConn lock()/unlock() methods for
**	    synchronization to support BLOB streams.
**	26-Oct-99 (gordy)
**	    Implemented ODBC escape processing.
**	12-Nov-99 (gordy)
**	    Use configured date formatter.
**	16-Nov-99 (gordy)
**	    Added query timeouts.
**	18-Nov-99 (gordy)
**	    Made Param, AscRdr, and UniRdr nested classes.
**	13-Dec-99 (gordy)
**	    Added fetch limit.
**	 2-Feb-00 (gordy)
**	    Send short streams as VARCHAR or VARBINARY.
**	 7-Feb-00 (gordy)
**	    For the NULL datatype or NULL data values, send the generic
**	    NULL datatype and the nullable indicator.
**	13-Jun-00 (gordy)
**	    Added support for derived classes (EdbcCall specfically).
**	 4-Oct-00 (gordy)
**	    Standardized internal tracing.
**	19-Oct-00 (gordy)
**	    Changes in transaction state result in prepared statements
**	    being lost.  The transaction state is now tracked and the
**	    query re-prepared when the state changes.
**	 1-Nov-00 (gordy)
**	    Added support for JDBC 2.0 extensions.  Added ChrRdr class.
**	15-Nov-00 (gordy)
**	    Extracted Param class to create a stand-alone Paramset class.
**	 8-Jan-01 (gordy)
**	    Extracted additional parameter handling to ParamSet.  Moved
**	    the BLOB processing classes.  Added support for batch processing.
**	23-Jan-01 (gordy)
**	    Allocate batch list on first access.
**	 5-Feb-01 (gordy)
**	    Coalesce statement IDs to preserve resources in the DBMS.
**	 3-Mar-01 (gordy)
**	    Centralize call to clearResults in exec().
**	11-Apr-01 (gordy)
**	    Replaced timezone specific date formatters with a single
**	    date formatter which, when synchronized, may be set to
**	    whatever timezone is required.  Do not include time with 
**	    DATE datatypes.
**	16-Apr-01 (gordy)
**	    Optimize usage of Result-set Meta-data.
**	10-May-01 (gordy)
**	    Added support for UCS2 datatypes.
**	 1-Jun-01 (gordy)
**	    Removed conversion of BLOB to fixed length type for short
**	    lengths.  JDBC requires long fixed length to BLOB conversion,
**	    but not the other way around.  Since BLOBs can only be
**	    specified explicitly, we now send in format requested.
**	13-Jun-01 (gordy)
**	    JDBC 2.1 spec requires UTF-8 encoding for unicode streams.
**	20-Jun-01 (gordy)
**	    Pass cursor name to result-set according to JDBC spec.
**	20-Aug-01 (gordy)
**	    Support UPDATE concurrency.  Send READONLY flag to server.
**	15-Feb-02 (gordy)
**	    Null parameter values should be mapped to a SQL NULL.
**	20-Feb-02 (gordy)
**	    Decimal not supported by gateways at proto level 0.
**	 6-Sep-02 (gordy)
**	    When describing character parameters, lengths should be
**	    for format sent to server (which is not necessarily the
**	    character length of the parameter).
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	19-Feb-03 (gordy)
**	    Updated to JDBC 3.0.
**	15-Apr-03 (gordy)
**	    Added connection timezones separate from date formatters.
**	26-Jun-03 (wansh01)
**	    Changed send_desc() and send_param() for Type.Binary to handle 
**	    null value properly. 
**	 7-Jul-03 (gordy)
**	    Added Ingres timezone config which affects date/time literals.
**	 4-Aug-03 (gordy)
**	    New result-set for updatable cursor.  Simplified pre-fetch 
**	    calculations.
**	20-Aug-03 (gordy)
**	    Add statement AUTO_CLOSE optimization.
**	12-Sep-03 (gordy)
**	    Replaced timezones with GMT indicator and SqlDates utility.
**	 6-Oct-03 (gordy)
**	    First block of rows may now be fetched with query response.
**	 1-Nov-03 (gordy)
**	    Implemented updatable result-sets.  ParamSet extended to
**	    provide most parameter handling functionality.
**      28-Apr-04 (wansh01)
**          Added Type as a input parameter of setObejct().
**	 5-May-04 (gordy)
**	    Further prepared statement optimizations.
**	14-Sep-05 (gordy)
**	    Save stream parameters shorter than the max variable length
**	    as fixed length types rather than BLOBs.
**	30-Jun-06 (gordy)
**	    Implemented support for DESCRIBE INPUT.
**	15-Sep-06 (gordy)
**	    Support empty date strings using alternate storage format.
**	10-Nov-06 (gordy)
**	    Prepared statement cache enhanced to maintain associations
**	    with active statements.
**	15-Nov-06 (gordy)
**	    Support LOB Locators via BLOB/CLOB.
**	30-Jan-07 (gordy)
**	    Backward compatibility for LOB Locators.
**	26-Feb-07 (gordy)
**	    Additional LOB Locator configuration capabilities.
**	 6-Apr-07 (gordy)
**	    Support scrollable cursors.
**	 4-May-07 (gordy)
**	    Set class access for reflection.
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
**	16-Mar-09 (gordy)
**	    Cleanup handling of internal LOBs.
**	30-Mar-09 (rajus01) SIR 121238
**	    Removed the prior SetNString()implementation. Most of the new 
**	    methods merely invokes other equivalent methods implemented 
**	    in this interface already. This is due to the fact that 
**	    these new NCS types are aliased. 
**	25-Mar-10 (gordy)
**	    Implement support for actual batch processing.
*/

import	java.math.BigDecimal;
import	java.net.URL;
import	java.util.LinkedList;
import	java.util.Calendar;
import	java.util.NoSuchElementException;
import	java.io.InputStream;
import	java.io.Reader;
import	java.io.InputStreamReader;
import	java.io.IOException;
import	java.io.UnsupportedEncodingException;
import	java.sql.Statement;
import	java.sql.PreparedStatement;
import	java.sql.ParameterMetaData;
import	java.sql.ResultSetMetaData;
import	java.sql.ResultSet;
import	java.sql.Types;
import	java.sql.Date;
import	java.sql.Time;
import	java.sql.Timestamp;
import	java.sql.Array;
import	java.sql.Blob;
import	java.sql.Clob;
import	java.sql.Ref;
import	java.sql.RowId;
import	java.sql.NClob;
import	java.sql.SQLXML;
import	java.sql.SQLException;
import  java.sql.BatchUpdateException;
import	java.sql.DataTruncation;
import	com.ingres.gcf.util.DbmsConst;
import  com.ingres.gcf.util.SqlData;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.util.SqlWarn;


/*
** Name: JdbcPrep
**
** Description:
**	JDBC driver class which implements the JDBC PreparedStatement 
**	interface.
**
**  Interface Methods:
**
**	executeQuery	    Execute a statement with result set.
**	executeUpdate	    Execute a statement with row count.
**	execute		    Execute a statement and indicate result.
**	addBatch	    Add parameters to batch set.
**	executeBatch	    Execute statement for each parameter set.
**	close		    Close the statement.
**	getMetaData	    Retrieve the ResultSet meta-data.
**	getParameterMetaData	Retrieves parameter meta-data.
**	clearParameters	    Release parameter resources.
**	setNull		    Set parameter to NULL.
**	setBoolean	    Set a boolean parameter value.
**	setByte		    Set a byte parameter value.
**	setShort	    Set a short parameter value.
**	setInt		    Set an int parameter value.
**	setLong		    Set a long parameter value.
**	setFloat	    Set a float parameter value.
**	setDouble	    Set a double parameter value.
**	setBigDecimal	    Set a BigDecimal parameter value.
**	setString	    Set a String parameter value.
**	setBytes	    Set a byte array parameter value.
**	setDate		    Set a Date parameter value.
**	setTime		    Set a Time parameter value.
**	setTimestamp	    Set a Timestamp parameter value.
**	setBinaryStream	    Set a binary stream parameter value.
**	setAsciiStream	    Set an ASCII stream parameter value.
**	setUnicodeStream    Set a Unicode stream parameter value.
**	setCharacterStream  Set a character stream parameter value.
**	setObject	    Set a generic Java object parameter value.
**	setArray	    Set an Array parameter value.
**	setBlob		    Set a Blob parameter value.
**	setClob		    Set a Clob parameter value.
**	setRef		    Set a Ref parameter value.
**	setURL		    Set a URL parameter value.
**
**  Protected Methods:
**
**	paramMap	    Map extern param index to internal index.
**	readDesc	    Override to re-use RSMD.
**
**  Protected Data:
**
**	params		    Parameter info.
**
**  Private Methods:
**
**	iterateBatch	    Execute individual batch entries.
**	exec		    Execute a prepared statement.
**
**  Private Data:
**
**	query_text	    Processed query text.
**	prepQPMD	    Parameter meta-data.
**	prepRSMD	    ResultSet meta-data.
**	emptyRSMD	    Empty ResultSet meta-data.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	17-Dec-99 (gordy)
**	    Added max_vchr_len to adapt to actual DBMS maximum.
**	13-Jun-00 (gordy)
**	    Added constructor for derived classes, a descriptor
**	    flag value for 'set' parameters, overriding methods
**	    for EdbcStmt 'execute' methods which aren't supported,
**	    and support for parameter names and flags.
**	19-Oct-00 (gordy)
**	    Added query_text, xactID and extracted prepare() from
**	    the constructor so that changes in transaction state
**	    can be tracked and the query re-prepared when necessary.
**	 1-Nov-00 (gordy)
**	    Support JDBC 2.0 extensions.  Added empty, getMetaData(), 
**	    addBatch(), setNull(), setDate(), setTime(), setTimestamp(),
**	    setCharacterStream(), setArray(), setBlob(), setClob(), setRef().
**	15-Nov-00 (gordy)
**	    Extracted Param class to create a stand-alone Paramset class.
**	    Removed ARRAY_INC and param_cnt, setFlags(), setName().  
**	    Parameters now implemented as ParamSet rather than Param array.
**	 8-Jan-01 (gordy)
**	    Extracted additional methods to ParamSet class.  Moved the
**	    BLOB processing classes.  Default parameter flags now saved
**	    in ParamSet.  Added addBatch(), executeBatch().
**	16-Apr-01 (gordy)
**	    Maintain a single RSMD (reloading as necessary).  
**	    Don't allocate empty RSMD until needed.
**	20-Aug-01 (gordy)
**	    Add qry_concur to hold query text concurrency for execution.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.  Added max_char_len, max_byte_len,
**	    max_vbyt_len rather than use max_vchr_len for all types.
**	25-Feb-03 (gordy)
**	    Added close method to clear parameter set, useAltStorage() to
**	    determine how parameters are stored in the parameter set,
**	    getParameterMetaData().
**	 1-Nov-03 (gordy)
**	    Moved send_desc(), send_params() and useAltStorage() to ParamSet,
**	    max column lengths to DrvConn, and added table_name for updatable 
**	    result-sets.  Implemented setObject() without scale parameter
**	    because ParamSet does not overload scale with negative values.
**	 5-May-04 (gordy)
**	    Additional optimizations for prepared statements.  Moved query
**	    text processing and statement preparation to separate class to
**	    share across all instances (not just statement name).  Removed 
**	    stmt_name, table_name, qry_concur and prepare() as these are now 
**	    supported by the DrvPrep class.  Removed xactID since prepared 
**	    statement tracking is done by getPrepStmt().  Made empty RSMD 
**	    non-static since tracing is connection oriented.
**	30-Jun-06 (gordy)
**	    Added prepQPMD for described input parameters.
**	10-Nov-06 (gordy)
**	    Added finalize() method to ensure clean-up of resources.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, permit access.
**	25-Mar-10 (gordy)
**	    Moved individual batch statement execution to iterateBatch()
**	    and added BatchExec class for actual batch execution.
*/

public class
JdbcPrep
    extends	JdbcStmt
    implements	PreparedStatement, DbmsConst
{

    protected ParamSet	params;

    private String	query_text = null;
    private JdbcQPMD	prepQPMD = null;
    private JdbcRSMD	prepRSMD = null;
    private JdbcRSMD	emptyRSMD = null;
    

/*
** Name: JdbcPrep
**
** Description:
**	Class constructor.  Primary constructor for this class
**	and derived classes which prepare the SQL text.
**
**	Caller is responsibile for ensuring that ResultSet type,
**	concurrency, and holdability are suitable for connection.
**
** Input:
**	conn	    Associated connection.
**	query	    Query text.
**	rs_type	    Default ResultSet type.
**	rs_concur   Cursor concurrency.
**	rs_hold	    Default ResultSet holdability.
**
** Output:
**	None.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	 8-Sep-99 (gordy)
**	    Synchronize on DbConn.
**	29-Sep-99 (gordy)
**	    Use DbConn lock()/unlock() methods for
**	    synchronization to support BLOB streams.
**	26-Oct-99 (gordy)
**	    Implemented ODBC escape processing.
**	12-Nov-99 (gordy)
**	    Use configured date formatter.
**	17-Dec-99 (gordy)
**	    Check DBMS max varch length.
**	13-Jun-00 (gordy)
**	    Extracted functionality for derived classes to
**	    new protected constructor.
**	 4-Oct-00 (gordy)
**	    Create unique ID for standardized internal tracing.
**	19-Oct-00 (gordy)
**	    Query prepare code extracted to prepare().  Process
**	    the query text then call new method to prepare query.
**	 1-Nov-00 (gordy)
**	    Changed parameters for JDBC 2.0 extensions.
**	31-Oct-01 (gordy)
**	    Timezone now passed to EdbcSQL.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	19-Feb-03 (gordy)
**	    Added ResultSet holdability.
**	15-Apr-03 (gordy)
**	    Added connection timezones separate from date formatters.
**	 7-Jul-03 (gordy)
**	    Added Ingres timezone config which affects date/time literals.
**	12-Sep-03 (gordy)
**	    Replaced timezones with GMT indicator and SqlDates utility.
**	 1-Nov-03 (gordy)
**	    Save updatable table name.
**	 5-May-04 (gordy)
**	    Statement processing and preparation moved to DrvPrep class.
**	10-Nov-06 (gordy)
**	    Prepared statement cache enhanced to maintain associations
**	    with active statements.
**	15-Nov-06 (gordy)
**	    Implemented getBlob() and getClob() to support DBMS LOB
**	    locators.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, restrict constructor access.
*/

// package access
JdbcPrep( DrvConn conn, String query, int rs_type, int rs_concur, int rs_hold )
    throws SQLException
{
    this( conn, rs_type, rs_concur, rs_hold );
    title = trace.getTraceName() + "-PreparedStatement[" + inst_id + "]";
    tr_id = "Prep[" + inst_id + "]";
    
    if ( trace.enabled( 2 ) )  trace.write( tr_id + ": '" + query + "'" );

    try
    {
	/*
	** Update prepared statement cache with query.
	** If this is a new query, the query text will
	** be processed and a new statement will be
	** prepared.  Otherwise, the existing statement
	** will be re-used.
	*/
	DrvPrep prepStmt = conn.createPrepStmt( query );
	query_text = query;
	prepRSMD = prepStmt.getMetaData();
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ": error preparing statement" );
	if ( trace.enabled( 1 ) )  SqlExFactory.trace( ex, trace );
	throw ex;
    }

    return;
} // JdbcPrep


/*
** Name: JdbcPrep
**
** Description:
**	Class constructor.  Common functionality for this class
**	and derived classes which don't actually prepare the
**	SQL text.
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
**	13-Jun-00 (gordy)
**	    Created.
**	 1-Nov-00 (gordy)
**	    Changed parameters for JDBC 2.0 extensions.
**	 8-Jan-01 (gordy)
**	    Set max varchar length in parameter set.
**	31-Oct-02 (gordy)
**	    Added max lengths for other column types.
**	19-Feb-03 (gordy)
**	    Added ResultSet holdability.  Pass CharSet to ParamSet.
**	12-Sep-03 (gordy)
**	    Replaced timezones with GMT indicator and SqlDates utility.
**	 1-Nov-03 (gordy)
**	    ParamSet funtionality extended.  Moved column lengths to DrvConn.
*/

protected
JdbcPrep( DrvConn conn, int rs_type, int rs_concur, int rs_hold )
{
    super( conn, rs_type, rs_concur, rs_hold );
    params = new ParamSet( conn );
    return;
} // JdbcPrep


/*
** Name: finalize
**
** Description:
**      Clean-up class instance.
**
** Input:
**      None.
**
** Output:
**      None.
**
** Returns:
**      void
**
** History:
**      10-Nov-06 (gordy)
**          Created.
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
** Name: executeQuery
**
** Description:
**	Execute an SQL prepared statement and return a result set.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	ResultSet   A new ResultSet object.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	 8-Jan-01 (gordy)
**	    Pass parameter set to exec() to support batch processing.
**	 3-Mar-01 (gordy)
**	    Centralize call to clearResults in exec().
*/

public ResultSet
executeQuery()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".executeQuery()" );

    if ( prepRSMD == null )  
    {
	if ( trace.enabled() )  
	    trace.log( title + ".executeQuery(): statement is not a query" );
	clearResults();
	throw SqlExFactory.get( ERR_GC4017_NO_RESULT_SET );
    }

    exec( params );
    if ( trace.enabled() )  trace.log(title + ".executeQuery(): " + resultSet);
    return( resultSet );
} // executeQuery


/*
** Name: executeUpdate
**
** Description:
**	Execute an SQL prepared statement and return a row count.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	    Row count or 0.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	 8-Jan-01 (gordy)
**	    Pass parameter set to exec() to support batch processing.
**	 3-Mar-01 (gordy)
**	    Centralize call to clearResults in exec().
*/

public int
executeUpdate()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".executeUpdate()" );

    if ( prepRSMD != null )  
    {
	if ( trace.enabled() )  
	    trace.log( title + ".executeUpdate(): statement is a query" );
	clearResults();
	throw SqlExFactory.get( ERR_GC4018_RESULT_SET_NOT_PERMITTED );
    }

    exec( params );
    int count = ((rslt_items & RSLT_ROW_CNT) != 0) ? rslt_val_rowcnt : 0;
    if ( trace.enabled() )  trace.log( title + ".executeUpdate(): " + count );
    return( count );
} // executeUpdate


/*
** Name: execute
**
** Description:
**	Execute an SQL prepared statement and return an indication 
**	as to the type of the result.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if the result is a ResultSet, false otherwise.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	 8-Jan-01 (gordy)
**	    Pass parameter set to exec() to support batch processing.
**	 3-Mar-01 (gordy)
**	    Centralize call to clearResults in exec().
*/

public boolean
execute()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".execute()" );
    exec( params );
    boolean isRS = (resultSet != null);
    if ( trace.enabled() )  trace.log( title + ".execute(): " + isRS );
    return( isRS );
} // execute


/*
** The following methods override Statement methods which are 
** not allowed for PreparedStatement objects.
*/

public ResultSet
executeQuery( String sql )
    throws SQLException
{
    if ( trace.enabled() )  trace.log(title + ".executeQuery('" + sql + "')");
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // executeQuery

public int
executeUpdate( String sql )
    throws SQLException
{
    if ( trace.enabled() )  trace.log(title + ".executeUpdate('" + sql + "')");
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // executeUpdate

public int
executeUpdate( String sql, int autoGeneratedKeys )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".executeUpdate('" + 
					sql + "'," + autoGeneratedKeys + ")" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // executeUpdate

public int
executeUpdate( String sql, int columns[] )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".executeUpdate('" + sql + "'," + 
		   (columns == null ? "null" : "[" + columns.length + "]") + 
		   ")" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // executeUpdate

public int
executeUpdate( String sql, String columns[] )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".executeUpdate('" + sql + "'," + 
		   (columns == null ? "null" : "[" + columns.length + "]") + 
		   ")");
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // executeUpdate

public boolean 
execute( String sql )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".execute('" + sql + "' )" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // execute

public boolean 
execute( String sql, int autoGeneratedKeys )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".execute('" + 
					sql + "'," + autoGeneratedKeys + ")" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // execute

public boolean 
execute( String sql, int columns[] )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".execute('" + sql + "'," + 
		   (columns == null ? "null" : "[" + columns.length + "]") + 
		   ")" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // execute

public boolean 
execute( String sql, String columns[] )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".execute('" + sql + "'," + 
		   (columns == null ? "null" : "[" + columns.length + "]") + 
		   ")" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // execute


/*
** Name: addBatch
**
** Description:
**	Add parameters to batch set.  
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
**	11-Jan-01 (gordy)
**	    Created.
**	23-Jan-01 (gordy)
**	    Allocate batch list on first access.
**	31-Oct-02 (gordy)
**	    Super-class now provides newBatch() to initialize batch list.
**	 1-Nov-03 (gordy)
**	    ParamSet no longer clonable.  Save current and alloc new.
*/

public void
addBatch()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".addBatch()" );

    if ( prepRSMD != null )  
    {
	if ( trace.enabled() )  
	    trace.log( title + ".addBatch(): statement is a query" );
	throw SqlExFactory.get( ERR_GC4018_RESULT_SET_NOT_PERMITTED );
    }
    
    if ( batch == null )  newBatch();
    synchronized( batch ) 
    { 
	batch.addLast( params );
	params = new ParamSet( conn );
    }
    return;
} // addBatch


/*
** Name: executeBatch
**
** Description:
**	Execute the prepared statement for each parameter set
**	in the batch.
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
    DrvPrep	prepStmt;
    int		results[];

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
    {
	/*
	** Get the prepared statement object.  If the statement
	** is no longer prepared, it will be re-prepared here.
	*/
	try { prepStmt = conn.findPrepStmt( query_text ); }
	catch( SQLException ex )
	{
	    if ( trace.enabled() )
		trace.log( title + ": error preparing statement" );
	    if ( trace.enabled( 1 ) )  SqlExFactory.trace( ex, trace );
	    throw ex;
	}

	synchronized( batch )
	{
	    BatchExec batchEx = new BatchExec( conn );
	    try { results = batchEx.execute( batch, prepStmt.getStmtName() ); }
	    finally { batch.clear(); }
	}
    }

    return( results );
} // executeBatch


/*
** Name: iterateBatch
**
** Description:
**	Individually execute the prepared statement for each
**	parameter set in the batch.
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
**	11-Jan-01 (gordy)
**	    Created.
**	23-Jan-01 (gordy)
**	    Allocate batch list on first access.
**	 3-Mar-01 (gordy)
**	    Centralize call to clearResults in exec().
**	31-Oct-02 (gordy)
**	    Check for NULL batch instead of allocating.
**	25-Feb-03 (gordy)
**	    Use SUCCESS_NO_INFO rather than explicit integer literal.
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
		ParamSet ps = (ParamSet)batch.pollFirst();

		if ( trace.enabled() )  
		    trace.log( title + ".executeBatch[" + i + "] " );

		exec( ps );
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
**	Execute a prepared statement.
**
** Input:
**	param_set	Parameters
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	 8-Sep-99 (gordy)
**	    Synchronize on DbConn.
**	29-Sep-99 (gordy)
**	    Use DbConn lock()/unlock() methods for
**	    synchronization to support BLOB streams.
**	15-Nov-99 (gordy)
**	    Pass max row count and column length to result set.
**	16-Nov-99 (gordy)
**	    Added query timeouts.
**	13-Dec-99 (gordy)
**	    Added fetch limit.
**	19-Oct-00 (gordy)
**	    Re-prepare the statement if transaction state changed.
**	 8-Jan-01 (gordy)
**	    Parameter set passed as parameter to support batch processing.
**	 3-Mar-01 (gordy)
**	    Centralize call to clearResults in exec().
**	16-Apr-01 (gordy)
**	    RSMD may change between prepare and execute (nasty!).  We 
**	    can't do anything about the application seeing the change, 
**	    but we must use the execute RSMD to read the result-set.
**	10-May-01 (gordy)
**	    Only use execute RSMD if one actually arrives.
**	20-Jun-01 (gordy)
**	    Pass cursor name to result-set according to JDBC spec:
**	    NULL if READONLY and not provided by application.
**	20-Aug-01 (gordy)
**	    Send READONLY flag to server.  Warn if cursor mode changed
**	    to READONLY.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver. 
**	25-Feb-03 (gordy)
**	    Check that all contiguous parameters have been provided
**	    (we don't know actual dynamic parameter count, so we can't
**	    tell if sufficient parameters have been provided). 
**	 4-Aug-03 (gordy)
**	    Created neew result-set class for updatable cursors.
**	20-Aug-03 (gordy)
**	    Send AUTO_CLOSE flag for query statements.
**	 6-Oct-03 (gordy)
**	    Fetch first block of rows for read-only cursors.
**	 1-Nov-03 (gordy)
**	    Implemented updatable result-sets.  ParamSet now
**	    builds DESC/DATA messages.
**	 5-May-04 (gordy)
**	    Prepared statements now represented by DrvPrep class.
**	10-Nov-06 (gordy)
**	    Prepared statement cache enhanced to maintain associations
**	    with active statements.
**	15-Nov-06 (gordy)
**	    Request LOB Locators when opening a cursor.  Locators
**	    are not utilized for non-selects or with select loops
**	    (due to tuple stream conflict with locator requests).
**	30-Jan-07 (gordy)
**	    Check if locators are enabled.
**	26-Feb-07 (gordy)
**	    Super class provides method to test for valid state
**	    for LOB Locators.
**	 6-Apr-07 (gordy)
**	    Support scrollable cursors.
**	 3-Mar-09 (gordy)
**	    Check that scrollable cursors are enabled.
*/

private void
exec( ParamSet param_set )
    throws SQLException
{
    DrvPrep prepStmt;
    int	    param_cnt;

    clearResults();
    
    /*
    ** All dynamic parameters must be set.  We check that all
    ** contiguous parameters have been set, the DBMS will check
    ** that sufficient parameters have been provided.
    */
    param_cnt = param_set.getCount();

    for( int param = 0; param < param_cnt; param++ )
	if ( ! param_set.isSet( param ) )
	{
	    if ( trace.enabled( 1 ) )
		trace.write( tr_id + ": parameter not set: " + (param + 1) );
	    throw SqlExFactory.get( ERR_GC4020_NO_PARAM );
	}

    /*
    ** Get the prepared statement object.  If the statement
    ** is no longer prepared, it will be re-prepared here.
    */
    try
    {
	prepStmt = conn.findPrepStmt( query_text );
	prepRSMD = prepStmt.getMetaData();
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ": error preparing statement" );
	if ( trace.enabled( 1 ) )  SqlExFactory.trace( ex, trace );
	throw ex;
    }

    /*
    ** Execute statement or open cursor.
    */
    msg.lock();
    try
    {
	if ( prepRSMD == null )
	{
	    /*
	    ** Non-query statement (execute).
	    */
	    msg.begin( MSG_QUERY );
	    msg.write( MSG_QRY_EXS );
	    msg.write( MSG_QP_STMT_NAME );
	    msg.write( prepStmt.getStmtName() );
	    msg.done( (param_cnt <= 0) );

	    if ( param_cnt > 0 )
	    {
		param_set.sendDesc( false );
		param_set.sendData( true );
	    }

	    readResults( timeout, true );
	}
	else
	{
	    /*
	    ** Select statement (open a cursor).  Generate a 
	    ** cursor ID if not provided by the application.
	    */
	    int		concurrency = getConcurrency(
						prepStmt.getConcurrency() );
	    short	flags = 0;
	    boolean	needEOG = true;
	    String	stmt = prepStmt.getStmtName();
	    String	cursor = (crsr_name != null) 
				 ? crsr_name : conn.getUniqueID( crsr_prefix );

	    /*
	    ** Parser removes FOR READONLY clause because it isn't
	    ** a part of Ingres SELECT syntax.  Tell server that
	    ** cursor should be READONLY (kludge older protocol
	    ** by appending clause to statement name).
	    */
	    if ( concurrency == DRV_CRSR_READONLY )
		if ( conn.msg_protocol_level < MSG_PROTO_2 )
		    stmt += " for readonly";
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
	    msg.write( MSG_QRY_OCS );
	    
	    if ( flags != 0 )
	    {
		msg.write( MSG_QP_FLAGS );
		msg.write( (short)2 );
		msg.write(  flags );
	    }
	    
	    msg.write( MSG_QP_CRSR_NAME );
	    msg.write( cursor );
	    msg.write( MSG_QP_STMT_NAME );
	    msg.write( stmt );
	    msg.done( (param_cnt <= 0) );

	    if ( param_cnt > 0 )
	    {
		param_set.sendDesc( false );
		param_set.sendData( true );
	    }

	    /*
	    ** If no RSMD is provided for execute request,
	    ** use the prepared statement RSMD.
	    */
	    JdbcRSMD rsmd = readResults( timeout, needEOG );
	    if ( rsmd == null )  rsmd = prepRSMD;

	    /*
	    ** The cursor name is passed to the result-set 
	    ** for updatable cursors or if provided by the 
	    ** application (JDBC 2.1 API spec).
	    */
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
					 cursor, prepStmt.getTableName() );
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
		resultSet = new RsltScrUpd( conn, this, rsmd, rslt_val_stmt, 
					    cursor, prepStmt.getTableName() );
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
	}
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".execute(): error executing query" );
	if ( trace.enabled( 1 ) )  SqlExFactory.trace( ex, trace );
	throw ex;
    }
    finally 
    { 
	msg.unlock(); 
    }

    return;
} // exec


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
**	25-Feb-03 (gordy)
**	    Created.
**	10-Nov-06 (gordy)
**	    Prepared statement cache enhanced to maintain associations
**	    with active statements.
*/

public void
close()
    throws SQLException
{
    /*
    ** Query text is associated with a connection prepared
    ** statement object which must be released.  Once the
    ** association is dropped, clear the query text to
    ** indicate association no longer exists.
    */
    if ( query_text != null )
    {
	conn.dropPrepStmt( query_text );
	query_text = null;
    }

    super.close();
    params.clear( true );
    return;
} // close


/*
** Name: getMetaData
**
** Description:
**	Retrieves the ResultSet meta-data describing the ResultSet
**	returned by the prepared statement.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	ResultSetMetaData   The ResultSet meta-data.
**
** History:
**	 1-Nov-00 (gordy)
**	    Created.
**	16-Apr-01 (gordy)
**	    Allocate empty RSMD only when needed.
*/

public ResultSetMetaData
getMetaData()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getMetaData(): " + prepRSMD );
    
    if ( prepRSMD == null  &&  emptyRSMD == null )  
	emptyRSMD = new JdbcRSMD( (short)0, trace );
    
    return( (prepRSMD == null) ? emptyRSMD : prepRSMD );
} // getMetadata


/*
** Name: getParameterMetaData
**
** Description:
**	Returns meta-data object describing the input parameters.
**
**	Ingres does not support DESCRIBE INPUT.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	ParameterMetaData   Parameter meta-data.
**
** History:
**	19-Feb-03 (gordy)
**	    Created.
**	30-Jun-06 (gordy)
**	    Implemented DESCRIBE INPUT support.
**	10-Nov-06 (gordy)
**	    Prepared statement cache enhanced to maintain associations
**	    with active statements.
*/

public ParameterMetaData
getParameterMetaData()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getParameterMetaData()" );

    if ( prepQPMD == null )  
	try
	{
	    DrvPrep prepStmt = conn.findPrepStmt( query_text );
	    prepQPMD = new JdbcQPMD( conn, prepStmt.getStmtName() );
	}
	catch( SQLException ex )
	{
	    if ( trace.enabled() )  
		trace.log( title + ": error describing statement" );
	    if ( trace.enabled( 1 ) )  SqlExFactory.trace( ex, trace );
	    throw ex;
	}

    if ( trace.enabled() )  
	trace.log( title + ".getParameterMetaData(): " + prepQPMD );

    return( prepQPMD );
} // getParameterMetaData


/*
** Name: clearParameters
**
** Description:
**	Clear stored parameter values and free resources.
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
**	19-May-99 (gordy)
**	    Created.
**	 1-Nov-03 (gordy)
**	    Synchronization handled by ParamSet.
*/

public void
clearParameters()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".clearParameters()" );
    params.clear( false );
    return;
} // clearParameters


/*
** Name: setNull
**
** Description:
**	Set parameter to be a NULL value.  The parameter is assigned
**	a NULL value of the requested SQL type.
**
** Input:
**	paramIndex	Parameter index.
**	sqlType		Column SQL type (java.sql.Types).
**	typeName	Name of SQL type (ignored).
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	13-Jun-00 (gordy)
**	    Map parameter index.
**	 1-Nov-00 (gordy)
**	    Added typeName (ignored) parameter.
**	 8-Jan-01 (gordy)
**	    Set method moved to ParamSet.
**	19-Feb-03 (gordy)
**	    Check type for alternate storage format.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setNull( int paramIndex, int sqlType, String typeName )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setNull( " + paramIndex + ", " + sqlType + 
		   (typeName == null ? " )" : ", " + typeName + " )") );

    paramIndex = paramMap( paramIndex );
    params.init( paramIndex, sqlType );
    params.setNull( paramIndex );
    return;
} // setNull


/*
** Name: setBoolean
**
** Description:
**	Set parameter to a boolean value.
**
** Input:
**	paramIndex	Parameter index.
**	value		Parameter value.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	13-Jun-00 (gordy)
**	    Map parameter index.
**	 8-Jan-01 (gordy)
**	    Set method moved to ParamSet.
**	19-Feb-03 (gordy)
**	    Replaced BIT with BOOLEAN.  
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setBoolean( int paramIndex, boolean value )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log(title + ".setBoolean( " + paramIndex + ", " + value + " )");

    paramIndex = paramMap( paramIndex );
    params.init( paramIndex, Types.BOOLEAN );
    params.setBoolean( paramIndex, value );
    return;
} // setBoolean


/*
** Name: setByte
**
** Description:
**	Set parameter to a byte value.
**
** Input:
**	paramIndex	Parameter index.
**	value		Byte value.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	13-Jun-00 (gordy)
**	    Map parameter index.
**	 8-Jan-01 (gordy)
**	    Set method moved to ParamSet.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setByte( int paramIndex, byte value )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setByte( " + paramIndex + ", " + value + " )" );
    
    paramIndex = paramMap( paramIndex );
    params.init( paramIndex, Types.TINYINT );
    params.setByte( paramIndex, value );
    return;
} // setByte


/*
** Name: setShort
**
** Description:
**	Set parameter to a short value.
**
** Input:
**	paramIndex	Parameter index.
**	value		Short value.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	13-Jun-00 (gordy)
**	    Map parameter index.
**	 8-Jan-01 (gordy)
**	    Set method moved to ParamSet.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setShort( int paramIndex, short value )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setShort( " + paramIndex + ", " + value + " )" );
    
    paramIndex = paramMap( paramIndex );
    params.init( paramIndex, Types.SMALLINT );
    params.setShort( paramIndex, value );
    return;
} // setShort


/*
** Name: setInt
**
** Description:
**	Set parameter to an int value.
**
** Input:
**	paramIndex	Parameter index.
**	value		Int value.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	13-Jun-00 (gordy)
**	    Map parameter index.
**	 8-Jan-01 (gordy)
**	    Set method moved to ParamSet.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setInt( int paramIndex, int value )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + " setInt( " + paramIndex + ", " + value + " )" );

    paramIndex = paramMap( paramIndex );
    params.init( paramIndex, Types.INTEGER );
    params.setInt( paramIndex, value );
    return;
} // setInt


/*
** Name: setLong
**
** Description:
**	Set parameter to a long value.
**
** Input:
**	paramIndex	Parameter index.
**	value		Long value.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	13-Jun-00 (gordy)
**	    Map parameter index.
**	 8-Jan-01 (gordy)
**	    Set method moved to ParamSet.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setLong( int paramIndex, long value )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setLong( " + paramIndex + ", " + value + " )" );

    paramIndex = paramMap( paramIndex );
    params.init( paramIndex, Types.BIGINT );
    params.setLong( paramIndex, value );
    return;
} // setLong


/*
** Name: setFloat
**
** Description:
**	Set parameter to a float value.
**
** Input:
**	paramIndex	Parameter index.
**	value		Float value.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	13-Jun-00 (gordy)
**	    Map parameter index.
**	 8-Jan-01 (gordy)
**	    Set method moved to ParamSet.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setFloat( int paramIndex, float value )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setFloat( " + paramIndex + ", " + value + " )" );

    paramIndex = paramMap( paramIndex );
    params.init( paramIndex, Types.REAL );
    params.setFloat( paramIndex, value );
    return;
} // setFloat


/*
** Name: setDouble
**
** Description:
**	Set parameter to a double value.
**
** Input:
**	paramIndex	Parameter index.
**	value		Double value.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	13-Jun-00 (gordy)
**	    Map parameter index.
**	 8-Jan-01 (gordy)
**	    Set method moved to ParamSet.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setDouble( int paramIndex, double value )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setDouble( " + paramIndex + ", " + value + " )" );

    paramIndex = paramMap( paramIndex );
    params.init( paramIndex, Types.DOUBLE );
    params.setDouble( paramIndex, value );
    return;
} // setDouble


/*
** Name: setBigDecimal
**
** Description:
**	Set parameter to a BigDecimal value.
**
** Input:
**	paramIndex	Parameter index.
**	value		BigDecimal value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	13-Jun-00 (gordy)
**	    Map parameter index.
**	 8-Jan-01 (gordy)
**	    Set method moved to ParamSet.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setBigDecimal( int paramIndex, BigDecimal value )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setBigDecimal( " + paramIndex + ", " + value + " )" );

    paramIndex = paramMap( paramIndex );
    params.init( paramIndex, Types.DECIMAL );
    params.setBigDecimal( paramIndex, value );
    return;
} // setBigDecimal


/*
** Name: setString
**
** Description:
**	Set parameter to a String value.
**
** Input:
**	paramIndex	Parameter index.
**	value		String value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	13-Jun-00 (gordy)
**	    Map parameter index.
**	 8-Jan-01 (gordy)
**	    Set method moved to ParamSet.
**	19-Feb-03 (gordy)
**	    Check for alternate storage format.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setString( int paramIndex, String value )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setString( " + paramIndex + ", " + value + " )" );

    paramIndex = paramMap( paramIndex );
    params.init( paramIndex, Types.VARCHAR );
    params.setString( paramIndex, value );
    return;
} // setString

/*
** Name: setBytes
**
** Description:
**	Set parameter to a byte array value.
**
** Input:
**	paramIndex	Parameter index.
**	value		Byte array value.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	13-Jun-00 (gordy)
**	    Map parameter index.
**	 8-Jan-01 (gordy)
**	    Set method moved to ParamSet.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Clean up tracing of array parameter.
*/

public void
setBytes( int paramIndex, byte value[] )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setBytes( " + paramIndex + ", " + 
		   (value == null ? "null" : "[" + value.length + "]") + 
		   " )" );

    paramIndex = paramMap( paramIndex );
    params.init( paramIndex, Types.VARBINARY );
    params.setBytes( paramIndex, value );
    return;
} // setBytes


/*
** Name: setDate
**
** Description:
**	Set parameter to a Date value.
**
**	If the connection is not using the GMT timezone, the timezone
**	associated with the calendar parameter (or local timezone) is 
**	used.
**
** Input:
**	paramIndex	Parameter index.
**	value		Date value.
**	cal		Calendar for timezone, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	13-Jun-00 (gordy)
**	    Map parameter index.
**	 1-Nov-00 (gordy)
**	    Convert to string using connection timezone.
**	 8-Jan-01 (gordy)
**	    Set method moved to ParamSet.
**	11-Apr-01 (gordy)
**	    Ingres supports (to some extent) dates without time, so do not
**	    include time with DATE datatypes.  Replaced timezone specific 
**	    date formatters with a single date formatter which, when 
**	    synchronized, may be set to whatever timezone is required.
**	15-Apr-03 (gordy)
**	    Abstracted date formatters.
**	12-Sep-03 (gordy)
**	    Replaced timezones with GMT indicator and SqlDates utility.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setDate( int paramIndex, Date value, Calendar cal )
    throws SQLException
{
    if ( trace.enabled() ) 
	trace.log( title + ".setDate( " + paramIndex + ", " + value + 
		   ((cal == null) ? " )" : ", " + cal + " )") );

    paramIndex = paramMap( paramIndex );
    params.init( paramIndex, Types.DATE );
    params.setDate( paramIndex, value, 
		    (cal == null) ? null : cal.getTimeZone() );
    return;
} // setDate


/*
** Name: setTime
**
** Description:
**	Set parameter to a Time value.
**
**	If the connection is not using the GMT timezone, the timezone
**	associated with the calendar parameter is used.
**
** Input:
**	paramIndex	Parameter index.
**	value		Time value.
**	cal		Calendar for timezone, may be null.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	13-Jun-00 (gordy)
**	    Map parameter index.
**	 1-Nov-00 (gordy)
**	    Convert to string using connection timezone.
**	 8-Jan-01 (gordy)
**	    Set method moved to ParamSet.
**	11-Apr-01 (gordy)
**	    Replaced timezone specific date formatters with a single
**	    date formatter which, when synchronized, may be set to
**	    whatever timezone is required.
**	15-Apr-03 (gordy)
**	    Abstracted date formatters.
**	21-Jul-03 (gordy)
**	    Apply calendar generically before connection based processing.
**	12-Sep-03 (gordy)
**	    Replaced timezones with GMT indicator and SqlDates utility.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setTime( int paramIndex, Time value, Calendar cal )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setTime( " + paramIndex + ", " + value + 
		   ((cal == null) ? " )" : ", " + cal + " )") );

    paramIndex = paramMap( paramIndex );
    params.init( paramIndex, Types.TIME );
    params.setTime( paramIndex, value,
		    (cal == null) ? null : cal.getTimeZone() );
    return;
} // setTime


/*
** Name: setTimestamp
**
** Description:
**	Set parameter to a Timestamp value.
**
**	If the connection is not using the GMT timezone, the timezone
**	associated with the calendar parameter is used.
**
** Input:
**	paramIndex	Parameter index.
**	value		Timestamp value.
**	cal		Calendar for timezone, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	13-Jun-00 (gordy)
**	    Map parameter index.
**	 1-Nov-00 (gordy)
**	    Convert to string using connection timezone.
**	 8-Jan-01 (gordy)
**	    Set method moved to ParamSet.
**	11-Apr-01 (gordy)
**	    Replaced timezone specific date formatters with a single
**	    date formatter which, when synchronized, may be set to
**	    whatever timezone is required.
**	15-Apr-03 (gordy)
**	    Abstracted date formatters.
**	21-Jul-03 (gordy)
**	    Apply calendar generically before connection based processing.
**	12-Sep-03 (gordy)
**	    Replaced timezones with GMT indicator and SqlDates utility.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setTimestamp( int paramIndex, Timestamp value, Calendar cal )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setTimestamp( " + paramIndex + ", " + value + 
		   ((cal == null) ? " )" : ", " + cal + " )") );

    paramIndex = paramMap( paramIndex );
    params.init( paramIndex, Types.TIMESTAMP );
    params.setTimestamp( paramIndex, value,
			 (cal == null) ? null : cal.getTimeZone() );
    return;
} // setTimestamp


/*
** Name: setBinaryStream
**
** Description:
**	Set parameter to a binary stream.
**
** Input:
**	paramIndex	Parameter index.
**	stream		Binary stream.
**	length		Length of stream in bytes.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	 2-Feb-00 (gordy)
**	    Send short streams as VARCHAR or VARBINARY.
**	13-Jun-00 (gordy)
**	    Map parameter index.
**	 8-Jan-01 (gordy)
**	    Stream classes moved to ParamSet.
**	 1-Jun-01 (gordy)
**	    No longer need wrapper class for binary InputStream.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
**	14-Sep-05 (gordy)
**	    If stream is short enough, save as VARBINARY.
*/

public void
setBinaryStream( int paramIndex, InputStream stream, long length )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".setBinaryStream( " + 
				       paramIndex + ", " + length + " )" );

    /*
    ** Check length to see if can be sent as VARBINARY.
    ** Ingres won't coerce BLOB to VARBINARY, but will
    ** coerce VARBINARY to BLOB, so VARBINARY is preferred.
    */
    if ( 
	 stream != null  &&
	 length >= 0  &&  
	 length <= conn.max_vbyt_len 
       )
    {
	byte	bytes[] = new byte[ (int)length ];

	try 
	{ 
	    int len = (length > 0) ? stream.read( bytes ) : 0; 

	    if ( len != length )
	    {
		/*
		** Byte array limits read so any difference
		** must be a truncation.
		*/
		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ".setBinaryStream: read only " +
				 len + " of " + length + " bytes!" );
		setWarning( new DataTruncation( paramIndex, true, 
						false, (int)length, len ) );
	    }
	}
	catch( IOException ex )
	{
	    throw SqlExFactory.get( ERR_GC4007_BLOB_IO ); 
	}
	finally
	{
	    try { stream.close(); }
	    catch( IOException ignore ) {}
	}
	
	paramIndex = paramMap( paramIndex );
	params.init( paramIndex, Types.VARBINARY );
	params.setBytes( paramIndex, bytes );
    }
    else
    {
	paramIndex = paramMap( paramIndex );
	params.init( paramIndex, Types.LONGVARBINARY );
	params.setBinaryStream( paramIndex, stream );
    }
    
    return;
} // setBinaryStream


/*
** Name: setAsciiStream
**
** Description:
**	Set parameter to an ASCII character stream.
**
** Input:
**	paramIndex	Parameter index.
**	stream		ASCII stream.
**	length		Length of stream in bytes.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	 2-Feb-00 (gordy)
**	    Send short streams as VARCHAR or VARBINARY.
**	13-Jun-00 (gordy)
**	    Map parameter index.
**	 8-Jan-01 (gordy)
**	    Stream classes moved to ParamSet.
**	 1-Jun-01 (gordy)
**	    No longer need wrapper class for Reader, so use Java
**	    wrapper class to convert InputStream to Reader.
**	19-Feb-03 (gordy)
**	    Check for alternate storage format.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
**	14-Sep-05 (gordy)
**	    If stream is short enough, save as VARCHAR.
**	15-Nov-05 (gordy)
**	    Close stream if used.
*/

public void
setAsciiStream( int paramIndex, InputStream stream, long length )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".setAsciiStream( " + 
				       paramIndex + ", " + length + " )" );

    /*
    ** Check length to see if can be sent as VARCHAR.
    ** Ingres won't coerce CLOB to VARCHAR, but will
    ** coerce VARCHAR to CLOB, so VARCHAR is preferred.
    */
    if ( 
	 stream != null  &&
	 length >= 0  &&  
	 length <= (conn.ucs2_supported ? conn.max_nvch_len 
					: conn.max_vchr_len) 
       )
    {
	char	chars[] = new char[ (int) length ];

	try 
	{ 
	    Reader	rdr = new InputStreamReader( stream, "US-ASCII" );
	    int		len = (length > 0) ? rdr.read( chars ) : 0; 

	    if ( len != length )
	    {
		/*
		** Character array limits read so any difference
		** must be a truncation.
		*/
		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ".setAsciiStream: read only " +
				 len + " of " + length + " characters!" );
		setWarning( new DataTruncation( paramIndex, true, 
						false, (int)length, len ) );
	    }
	}
	catch( IOException ex )
	{
	    throw SqlExFactory.get( ERR_GC4007_BLOB_IO ); 
	}
	finally
	{
	    try { stream.close(); }
	    catch( IOException ignore ) {}
	}

	paramIndex = paramMap( paramIndex );
	params.init( paramIndex, Types.VARCHAR );
	params.setString( paramIndex, new String( chars ) );
    }
    else
    {
	paramIndex = paramMap( paramIndex );
	params.init( paramIndex, Types.LONGVARCHAR );
	params.setAsciiStream( paramIndex, stream );
    }
    
    return;
} // setAsciiStream

/*
** Name: setUnicodeStream
**
** Description:
**	Set parameter to a Unicode character stream.
**
** Input:
**	paramIndex	Parameter index.
**	stream		Unicode stream.
**	length		Length of stream in bytes.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	 2-Feb-00 (gordy)
**	    Send short streams as VARCHAR or VARBINARY.
**	13-Jun-00 (gordy)
**	    Map parameter index.
**	 8-Jan-01 (gordy)
**	    Stream classes moved to ParamSet.
**	13-Jun-01 (gordy)
**	    JDBC 2.1 spec requires UTF-8 encoding for unicode streams.
**	19-Feb-03 (gordy)
**	    Check for alternate storage format.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setUnicodeStream( int paramIndex, InputStream stream, int length )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".setUnicodeStream( " + 
	paramIndex + ", " + length + " )" );

    paramIndex = paramMap( paramIndex );
    params.init( paramIndex, Types.LONGVARCHAR );
    params.setUnicodeStream( paramIndex, stream );
    return;
} // setUnicodeStream


/*
** Name: setCharacterStream
**
** Description:
**	Set parameter to a character stream.
**
** Input:
**	paramIndex	Parameter index.
**	reader		Character stream.
**	length		Length of stream in bytes.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 2-Nov-00 (gordy)
**	    Created.
**	 8-Jan-01 (gordy)
**	    Stream classes moved to ParamSet.
**	 1-Jun-01 (gordy)
**	    No longer need wrapper class for Reader.
**	19-Feb-03 (gordy)
**	    Check for alternate storage format.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
**	14-Sep-05 (gordy)
**	    If stream is short enough, save as VARCHAR.
**	15-Nov-06 (gordy)
**	    Close stream if used.
*/

public void
setCharacterStream( int paramIndex, Reader reader, long length )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".setCharacterStream( " + 
				       paramIndex + ", " + length + " )" );
    
    /*
    ** Check length to see if can be sent as VARCHAR.
    ** Ingres won't coerce CLOB to VARCHAR, but will
    ** coerce VARCHAR to CLOB, so VARCHAR is preferred.
    */
    if ( 
	 reader != null  &&
	 length >= 0  &&  
	 length <= (conn.ucs2_supported ? conn.max_nvch_len 
					: conn.max_vchr_len) 
       )
    {
	char	chars[] = new char[ (int)length ];

	try 
	{ 
	    int	len = (length > 0) ? reader.read( chars ) : 0; 

	    if ( len != length )
	    {
		/*
		** Character array limits read so any difference
		** must be a truncation.
		*/
		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ".setCharacterStream: read only " +
				 len + " of " + length + " characters!" );
		setWarning( new DataTruncation( paramIndex, true, 
						false, (int)length, len ) );
	    }
	}
	catch( IOException ex )
	{
	    throw SqlExFactory.get( ERR_GC4007_BLOB_IO ); 
	}
	finally
	{
	    try { reader.close(); }
	    catch( IOException ignore ) {}
	}

	paramIndex = paramMap( paramIndex );
	params.init( paramIndex, Types.VARCHAR );
	params.setString( paramIndex, new String( chars ) );
    }
    else
    {
	paramIndex = paramMap( paramIndex );
	params.init( paramIndex, Types.LONGVARCHAR );
	params.setCharacterStream( paramIndex, reader );
    }
    
    return;
} // setCharacterStream


/*
** Name: setBlob
**
** Description:
**	Set parameter to a Blob object.
**
** Input:
**	paramIndex	Parameter index.
**	value		Blob object.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	15-Nov-06 (gordy)
**	    Implemented.
**	16-Mar-09 (gordy)
**	    For internal LOBs, initialize parameter by value.
**	    Otherwise, initialize by type.
*/

public void
setBlob( int paramIndex, Blob blob )  
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setBlob(" + paramIndex + "," + blob + ")");

    /*
    ** If the Blob represents a locator associated with
    ** the current connection, then the parameter can be
    ** be treated as a BLOB type.  Otherwise, the Blob
    ** is handled generically as a LONGVARBINARY byte
    ** stream.
    */
    paramIndex = paramMap( paramIndex );

    if ( blob == null )
    {
	params.init( paramIndex, Types.BLOB );
	params.setNull( paramIndex );
    }
    else  if ( blob instanceof JdbcBlob  &&
	       ((JdbcBlob)blob).isValidLocator( conn ) )
    {
	DrvLOB lob = ((JdbcBlob)blob).getLOB();
	params.init( paramIndex, lob );

	if ( lob instanceof DrvBlob )
	    params.setBlob( paramIndex, (DrvBlob)lob );
	else	// Should not happen
	    params.setBinaryStream( paramIndex, blob.getBinaryStream() );
    }
    else
    {
	params.init( paramIndex, Types.LONGVARBINARY );
	params.setBinaryStream( paramIndex, blob.getBinaryStream() );
    }

    return;
} // setBlob


/*
** Name: setClob
**
** Description:
**	Set parameter to a Clob object.
**
** Input:
**	paramIndex	Parameter index.
**	value		Clob object.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	15-Nov-06 (gordy)
**	    Implemented.
**	16-Mar-09 (gordy)
**	    For internal LOBs, initialize parameter by value.
**	    Otherwise, initialize by type.
*/

public void
setClob( int paramIndex, Clob clob )  
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setClob(" + paramIndex + "," + clob + ")");

    /*
    ** If the Clob represents a locator associated with
    ** the current connection, then the parameter can be
    ** be treated as a CLOB type.  Otherwise, the Clob
    ** is handled generically as a LONGVARCHAR character
    ** stream.
    */
    paramIndex = paramMap( paramIndex );

    if ( clob == null )
    {
	params.init( paramIndex, Types.CLOB );
	params.setNull( paramIndex );
    }
    else  if ( clob instanceof JdbcClob  &&
	       ((JdbcClob)clob).isValidLocator( conn ) )
    {
	/*
	** Parameter alternate storage format is
	** determined by the type of the locator.
	*/
	DrvLOB lob = ((JdbcClob)clob).getLOB();
	params.init( paramIndex, lob );

	if ( lob instanceof DrvClob )
	    params.setClob( paramIndex, (DrvClob)lob );
	else  if ( lob instanceof DrvNlob )
	    params.setClob( paramIndex, (DrvNlob)lob );
	else	// Should not happen
	    params.setCharacterStream( paramIndex, clob.getCharacterStream() );
    }
    else
    {
	params.init( paramIndex, Types.LONGVARCHAR );
	params.setCharacterStream( paramIndex, clob.getCharacterStream() );
    }

    return;
} // setClob


/*
** Name: setObject
**
** Description:
**	Set parameter to a generic Java object.  SQL type and scale
**	are determined based on the class and value of the Java object.
**
** Input:
**	paramIndex	Parameter index.
**	value		Java object.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-03 (gordy)
**	    Created.
**	15-Nov-06 (gordy)
**	    Support LOB Locators.
**	16-Mar-09 (gordy)
**	    For internal LOBs, initialize parameter by value.
**	    Otherwise, initialize by type.
*/

public void
setObject( int paramIndex, Object value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log( title + ".setObject( " + paramIndex + " )" );
    
    /*
    ** Blob/Clob require special handling because they can represent 
    ** LOB locators as well as stream data.  If the LOB represents a 
    ** valid locator, extract the driver locator.  Otherwise, derive 
    ** a stream from the LOB.
    */
    if ( value != null )
	if ( value instanceof Blob )
	{
	    if ( value instanceof JdbcBlob  &&
		 ((JdbcBlob)value).isValidLocator( conn ) )
		value = ((JdbcBlob)value).getLOB();
	    else
		value = ((Blob)value).getBinaryStream();
	}
	else  if ( value instanceof Clob )
	{
	    if ( value instanceof JdbcClob  &&
		 ((JdbcClob)value).isValidLocator( conn ) )
		value = ((JdbcClob)value).getLOB();
	    else
		value = ((Clob)value).getCharacterStream();
	}

    paramIndex = paramMap( paramIndex );
    params.init( paramIndex, value );
    params.setObject( paramIndex, value );
    return;
} // setObject


/*
** Name: setObject
**
** Description:
**	Set parameter to a generic Java object with the requested
**	SQL type and scale.  A scale value of -1 indicates that the
**	scale of the value should be used.
**
**	Special handling of certain Java objects may be requested
**	by setting sqlType to OTHER.  If the object does not have
**	a processing alternative, the value will processed as if
**	setObject() with no SQL type was called (except that the
**	scale value provided will be used for DECIMAL values).
**
** Input:
**	paramIndex	Parameter index.
**	value		Java object.
**	sqlType		Target SQL type.
**	scale		Number of digits following decimal point, or -1.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	 8-Jan-01 (gordy)
**	    Set methods moved to ParamSet.
**	10-May-01 (gordy)
**	    Alternate datatype formats supported through OTHER.
**	19-Feb-03 (gordy)
**	    Check for default alternate storage format.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
**      28-Apr-04 (wansh01)
**          Passed input sqlType to setObejct().
**          For Types.OTHER, type of the object will be used for setObject().
**	15-Sep-06 (gordy)
**	    Support empty date strings.
**	15-Nov-06 (gordy)
**	    Support LOB Locators.
**	16-Mar-09 (gordy)
**	    For internal LOBs, initialize parameter by value.
**	    Otherwise, initialize by type.
*/

public void
setObject( int paramIndex, Object value, int sqlType, int scale )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".setObject( " + paramIndex + 
				       ", " + sqlType + ", " + scale + " )" );
    
    /*
    ** Blob/Clob require special handling because they can represent 
    ** LOB locators as well as stream data.  If the LOB represents a 
    ** valid locator, extract the driver locator.  Otherwise, derive 
    ** a stream from the value.
    */
    if ( value != null )
	if ( value instanceof Blob )
	{
	    if ( value instanceof JdbcBlob  &&
		 ((JdbcBlob)value).isValidLocator( conn ) )
		value = ((JdbcBlob)value).getLOB();
	    else
		value = ((Blob)value).getBinaryStream();
	}
	else  if ( value instanceof Clob )
	{
	    if ( value instanceof JdbcClob  &&
		 ((JdbcClob)value).isValidLocator( conn ) )
		value = ((JdbcClob)value).getLOB();
	    else
		value = ((Clob)value).getCharacterStream();
	}

    paramIndex = paramMap( paramIndex );
    
    switch( sqlType )
    {
    case Types.OTHER :
	/*
	** Force alternate storage format.
	*/
	sqlType = SqlData.getSqlType( value );
	params.init( paramIndex, value, true );
	break;

    case Types.DATE :
    case Types.TIME :
    case Types.TIMESTAMP :
	/*
	** Empty dates are only supported by Ingres dates
	** which require the alternate storage format.
	*/
	if ( 
	     value != null  &&
	     value instanceof String  &&  
	     ((String)value).length() == 0 
	   )
	    params.init( paramIndex, sqlType, true );
	else
	    params.init( paramIndex, sqlType );
	break;

    case Types.LONGVARBINARY :
    case Types.BLOB :
	/*
	** Internally, the BLOB type only supports driver locators.
	** LONGVARBINARY supports all other Blobs and coercions.
	*/
	if ( value == null )
	    params.init( paramIndex, sqlType );
	else  if ( value instanceof DrvBlob )  
	{
	    sqlType = Types.BLOB;
	    params.init( paramIndex, value );
	}
	else
	{
	    sqlType = Types.LONGVARBINARY;
	    params.init( paramIndex, sqlType );
	}
	break;

    case Types.LONGVARCHAR :
    case Types.CLOB :
	/*
	** Internally, the CLOB type only supports driver locators.
	** LONGVARCHAR supports all other Clobs and coercions.
	*/
	if ( value == null )  
	    params.init( paramIndex, sqlType );
	else  if ( value instanceof DrvClob  ||  value instanceof DrvNlob )  
	{
	    sqlType = Types.CLOB;
	    params.init( paramIndex, value );
	}
	else
	{
	    sqlType = Types.LONGVARCHAR;
	    params.init( paramIndex, sqlType );
	}
	break;

    default :
	params.init( paramIndex, sqlType );
	break;
    }
 
    params.setObject( paramIndex, value, sqlType, scale );
    return;
}


/*
** Data save methods which are simple covers 
** for the primary data save methods.
*/

public void 
setNull( int paramIndex, int sqlType ) throws SQLException
{ setNull( paramIndex, sqlType, null ); }

public void 
setDate( int paramIndex, Date value ) throws SQLException
{ setDate( paramIndex, value, null ); }

public void 
setTime( int paramIndex, Time value ) throws SQLException
{ setTime( paramIndex, value, null ); }

public void 
setTimestamp( int paramIndex, Timestamp value ) throws SQLException
{ setTimestamp( paramIndex, value, null ); }

public void 
setObject( int paramIndex, Object value, int sqlType ) throws SQLException
{ setObject( paramIndex, value, sqlType, 0 ); }

public void
setNString( int paramIndex, String value ) throws SQLException
{ setString( paramIndex, value ); }

public void
setCharacterStream(int paramIndex, Reader reader, int length) 
throws SQLException
{ setCharacterStream( paramIndex, reader, (long) length ); }

public void
setCharacterStream(int paramIndex, Reader reader) throws SQLException
{ setCharacterStream( paramIndex, reader, -1L ); }

public void
setClob(int paramIndex, Reader reader, long length) throws SQLException
{ setCharacterStream( paramIndex, reader, length ); }

public void
setClob(int paramIndex, Reader reader) throws SQLException
{ setCharacterStream( paramIndex, reader, -1L ); }

public void
setNCharacterStream(int paramIndex, Reader reader, long length)
throws SQLException
{ setCharacterStream( paramIndex, reader, length); }

public void
setNCharacterStream(int paramIndex, Reader value) throws SQLException
{ setCharacterStream( paramIndex, value, -1L ); }

public void
setNClob(int paramIndex, NClob value) throws SQLException
{ setClob( paramIndex, (Clob) value ); }

public void
setNClob(int paramIndex, Reader reader, long length)
throws SQLException
{ setCharacterStream( paramIndex, reader, length); }

public void
setNClob(int paramIndex, Reader reader)
throws SQLException
{ setCharacterStream( paramIndex, reader, -1L ); }

public void
setAsciiStream(int paramIndex, InputStream stream, int length)
throws SQLException
{ setAsciiStream( paramIndex, stream, (long) length ); }

public void
setAsciiStream(int paramIndex, InputStream stream) throws SQLException
{ setAsciiStream( paramIndex, stream, -1L); }

public void
setBinaryStream( int paramIndex, InputStream stream, int length )
throws SQLException
{ setBinaryStream( paramIndex, stream, (long) length ); }

public void
setBinaryStream(int paramIndex, InputStream stream) throws SQLException
{ setBinaryStream( paramIndex, stream, -1L); }

public void
setBlob(int paramIndex, InputStream stream, long length)
throws SQLException
{ setBinaryStream( paramIndex, stream, length); }

public void
setBlob(int paramIndex, InputStream stream) throws SQLException
{ setBinaryStream( paramIndex, stream, -1L); }


/*
** Data save methods which are for unsupported types.
*/

public void
setArray( int paramIndex, Array array )  throws SQLException
{
    if ( trace.enabled() )  trace.log(title + ".setArray(" + paramIndex + ")");
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // setArray

public void
setRef( int paramIndex, Ref ref )  throws SQLException
{
    if ( trace.enabled() )  trace.log(title + ".setRef(" + paramIndex + ")");
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // setRef

public void
setURL( int paramIndex, URL url )  throws SQLException
{
    if ( trace.enabled() )  trace.log(title + ".setURL(" + paramIndex + ")");
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // setURL

public void
setRowId(int paramIndex, RowId rid) throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".setRowId( " + paramIndex + ", " + rid + " )" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // setRowId

public void
setSQLXML(int paramIndex, SQLXML xmlObject) throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".setSQLXML( " + paramIndex + ")" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // setSQLXML

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

 } // isWrapperFor

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
**      24-Nov-08 (rajus01)
**         Created.
**      23-Jun-09 (rajus01)
**         Implemented.
*/
public <T> T
unwrap(Class<T> iface) throws SQLException
{
    if ( trace.enabled() )
       trace.log(title + ".Unwrap(" + iface + ")");
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
} // unwrap


/*
** Name: paramMap
**
** Description:
**	Determines if a given parameter index is valid
**	and maps the external index to the internal index.
**	Ensures the internal parameter array is prepared
**	to hold a parameter for the given index.
**
** Input:
**	index	    External parameter index >= 1.
**
** Output:
**	None.
**
** Returns:
**	int	    Internal parameter index >= 0.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	13-Jun-00 (gordy)
**	    Use System method to copy array and Param method
**	    to reset the parameter values.
*/

protected int
paramMap( int index )
    throws SQLException
{
    if ( index < 1 )  throw SqlExFactory.get( ERR_GC4011_INDEX_RANGE );
    return( index - 1 );
} // paramMap


/*
** Name: readDesc
**
** Description:
**	Read a query result data descriptor message.  Overrides the
**	method in JdbcStmt.  Maintains a single RSMD.  Subsequent
**	descriptor messages are loaded into the existing RSMD.
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
**	16-Apr-01 (gordy)
**	    Created.
**	 5-May-04 (gordy)
**	    Declare and use own local RSMD rather than prepared statement 
**	    RSMD.  Call super-class to properly handle timeouts.
*/

    private JdbcRSMD	localRSMD = null;
    
protected JdbcRSMD
readDesc()
    throws SQLException
{
    return( localRSMD = readDesc( localRSMD ) );
} // readDesc


/*
** Name: BatchExec
**
** Description:
**	Class which implements batch query execution.
**
**  Public Methods:
**
**	execute		Execute batch.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

protected static class
BatchExec
    extends JdbcStmt.BatchExec
{

/*
** Name: BatchExec
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
**	25-Mar-10 (gordy)
**	    Created.
*/

public
BatchExec( DrvConn conn )
{ super( conn ); }


/*
** Name: execute
**
** Description:
**	Execute a prepared statement with a batch of parameter sets.
**	
**
** Input:
**	batch		List of parameter sets.
**	stmtName	Prepared statement name.
**
** Output:
**	None.
**
** Returns:
**	int []		Batch query results.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

public int []
execute( LinkedList batch, String stmtName )
    throws SQLException
{
    if ( batch == null  ||  batch.peekFirst() == null )  return( noResults );

    clearResults();
    msg.lock();

    try
    {
	boolean		first = true;
	int		count = 0;
	ParamSet	ps;

	/*
	** Send each parameter set to batch server.
	*/
	while( (ps = (ParamSet)batch.pollFirst()) != null )
	{
	    int		paramCount = ps.getCount();
	    byte	hdrFlag = (batch.peekFirst() == null) ? MSG_HDR_EOG 
							      : MSG_HDR_EOB;

	    for( int param = 0; param < paramCount; param++ )
		if ( ! ps.isSet( param ) )
		{
		    if ( trace.enabled( 1 ) )
			trace.write( tr_id + ": parameter not set: "
					   + (param + 1) );
		    throw SqlExFactory.get( ERR_GC4020_NO_PARAM );
		}

	    if ( trace.enabled() )
		trace.log( title + ".executeBatch[" + count + "]" );

	    count++;
	    msg.begin( MSG_BATCH );
	    msg.write( MSG_BAT_EXS );

	    if ( first )
	    {
		msg.write( MSG_BP_STMT_NAME );
		msg.write( stmtName );
		first = false;
	    }
	    else
	    {
		msg.write( MSG_BP_FLAGS );
		msg.write( (short)2 );
		msg.write( MSG_BF_REPEAT );
	    }

	    if ( paramCount <= 0 )
		msg.done( hdrFlag );
	    else
	    {
		msg.done( false );
		ps.sendDesc( false );
		ps.sendData( hdrFlag );
	    }
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


} // class BatchExec


} // class JdbcPrep
