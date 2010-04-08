/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbc;

/*
** Name: EdbcPrep.java
**
** Description:
**	Implements the EDBC JDBC PreparedStatement class: EdbcPrep.
**	Defines classes for handling BLOB input streams.
**
**  Classes:
**
**	EdbcPrep
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
**      10-Apr-03 (weife01) BUG 110037
**          Added parseConnDate and formatConnDate to synchronize the
**          DateFormat returned by getConnDateFormat
*/

import	java.math.BigDecimal;
import	java.text.DateFormat;
import	java.util.LinkedList;
import	java.util.Calendar;
import	java.util.NoSuchElementException;
import	java.io.InputStream;
import	java.io.ByteArrayInputStream;
import	java.io.Reader;
import	java.io.InputStreamReader;
import	java.io.CharArrayReader;
import	java.io.StringReader;
import	java.io.UnsupportedEncodingException;
import	java.sql.PreparedStatement;
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
import	java.sql.SQLException;
import  java.sql.BatchUpdateException;
import	ca.edbc.util.EdbcEx;
import	ca.edbc.io.DbConn;


/*
** Name: EdbcPrep
**
** Description:
**	EDBC JDBC Java driver class which implements the
**	JDBC PreparedStatement interface.
**
**  Interface Methods:
**
**	executeQuery	    Execute a statement with result set.
**	executeUpdate	    Execute a statement with row count.
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
**	setAsciiStream	    Set an ASCII stream parameter value.
**	setUnicodeStream    Set a Unicode stream parameter value.
**	setBinaryStream	    Set a binary stream parameter value.
**	clearParameters	    Release parameter resources.
**	setObject	    Set a generic Java object parameter value.
**	execute		    Execute a statement and indicate result.
**	addBatch	    Add parameters to batch set.
**	executeBatch	    Execute statement for each parameter set.
**	setRef		    Set a Ref parameter value.
**	setBlob		    Set a Blob parameter value.
**	setClob		    Set a Clob parameter value.
**	setArray	    Set an Array parameter value.
**	getMetaData	    Retrieve the ResultSet meta-data.
**
**  Protected Methods:
**
**	send_desc	    Send DESC message.
**	send_params	    Send DATA message.
**	paramMap	    Map extern param index to internal index.
**
**  Protected Data:
**
**	params		    Parameter info.
**
**  Private Methods:
**
**	prepare		    Prepare the query.
**	exec		    Execute a prepared statement.
**
**  Private Data:
**
**	query_text	    Processed query text.
**	qry_concur	    Query concurrency.
**	xactID		    Transaction ID of prepared statement.
**	stmt_name	    Statement name.
**	rsmd		    Result set meta-data for statement.
**	max_vchr_len	    Max varchar string length.
**	empty		    Empty RSMD for non-select statements.
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
*/

class
EdbcPrep
    extends	EdbcStmt
    implements	PreparedStatement
{

    protected ParamSet	params;

    private String	query_text = null;
    private int		qry_concur = EdbcSQL.CONCUR_DEFAULT;
    private int		xactID;
    private String	stmt_name = null;
    private EdbcRSMD	rsmd = null;
    private int		max_vchr_len = DBMS_COL_MAX;

    /*
    ** Empty RSMD for non-select statements.
    */
    private static	EdbcRSMD empty = null;

/*
** Name: EdbcPrep
**
** Description:
**	Class constructor.  Primary constructor for this class
**	and derived classes which prepare the SQL text.
**
** Input:
**	conn	    Associated connection.
**	query	    Query text.
**	rs_type	    Default ResultSet type.
**	rs_concur   Default ResultSet concurrency.
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
**	06-May-04 (rajus01) Bug # 112279; Star Problem #: EDBC63
**	    Prepare is not issued again if the query is already prepared 
**	    and it is not a SELECT query.
*/

// package access needed
EdbcPrep( EdbcConnect conn, String query, int rs_type, int rs_concur )
    throws EdbcEx
{
    this( conn, rs_type, rs_concur );
    title = shortTitle + "-PreparedStatement[" + inst_id + "]";
    tr_id = "Prep[" + inst_id + "]";
    int qry_type = -1;
    
    if ( trace.enabled() )
	trace.log( title + ": '" + query + "'" );

    try
    {
	EdbcSQL sql = new EdbcSQL( query, getConnDateFormat().getTimeZone() );
	query_text = sql.parseSQL( parse_escapes );
	qry_concur = sql.getConcurrency();
	qry_type = sql.getQueryType();
    }
    catch( EdbcEx ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ": error parsing query text" );
	if ( trace.enabled( 1 ) )  ex.trace( trace );
	throw ex;
    }
    

    if( dbc.getStmtID( query_text ) == null ||  
                      qry_type == EdbcSQL.QT_SELECT ) 
	prepare();
    else
    {
	rsmd = null;
	xactID = dbc.getXactID();
	stmt_name = dbc.getStmtID( query_text, stmt_prefix );
    }
} // EdbcPrep


/*
** Name: EdbcPrep
**
** Description:
**	Class constructor.  Common functionality for this class
**	and derived classes which don't actually prepare the
**	SQL text.
**
** Input:
**	conn	    Associated connection.
**	rs_type	    Default ResultSet type.
**	rs_concur   Default ResultSet concurrency.
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
*/

protected
EdbcPrep( EdbcConnect conn, int rs_type, int rs_concur )
    throws EdbcEx
{
    super( conn, rs_type, rs_concur );

    try
    {
	String value;

	if ( (value = dbc.getDbmsInfo( "SQL_MAX_VCHR_COLUMN_LEN" )) != null )  
	    max_vchr_len = Integer.parseInt( value );
    }
    catch( Exception ignore ) {}

    params = new ParamSet( getConnDateFormat() );
    params.setMaxLength( max_vchr_len );
}


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

    if ( query_text == null  ||  rsmd == null )  
    {
	if ( trace.enabled() )  
	    trace.log( title + ".executeQuery(): statement is not a query" );
	clearResults();
	throw EdbcEx.get( E_JD0008_NOT_QUERY );
    }

    exec( params );
    if ( trace.enabled() )  
	trace.log( title + ".executeQuery(): " + resultSet );
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

    if ( query_text == null  ||  rsmd != null )  
    {
	if ( trace.enabled() )  
	    trace.log( title + ".executeUpdate(): statement is not an update" );
	clearResults();
	throw EdbcEx.get( E_JD0009_NOT_UPDATE );
    }

    exec( params );
    if ( trace.enabled() )  
	trace.log( title + ".executeUpdate(): " + updateCount );
    return( updateCount );
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
    boolean isRS = false;
    if ( trace.enabled() )  trace.log( title + ".execute()" );
    exec( params );
    if ( resultSet != null )  isRS = true;
    if ( trace.enabled() )  trace.log( title + ".execute(): " + isRS );
    return( isRS );
}


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
*/

public void
addBatch()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".addBatch()" );

    if ( query_text == null  ||  rsmd != null )  
    {
	if ( trace.enabled() )  
	    trace.log( title + ".addBatch(): statement is not an update" );
	throw EdbcEx.get( E_JD0009_NOT_UPDATE );
    }
    
    if ( batch == null )  synchronized( this ) { batch = new LinkedList(); }
    synchronized( batch ) { batch.addLast( params.clone() ); }
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
		ParamSet ps = (ParamSet)batch.removeFirst();
		if ( trace.enabled() )  
		    trace.log( title + ".executeBatch[" + i + "] " );

		exec( ps );
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
** Name: prepare
**
** Description:
**	Prepares the query text and assigns a unique statement
**	name and transaction ID.  If the query is a SELECT, a
**	result-set meta-data object is also produced.
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
**	19-Oct-00 (gordy)
**	    Extracted from constructor.
**	 5-Feb-01 (gordy)
**	    Coalesce statement IDs.
*/

private void
prepare()
    throws EdbcEx
{
    xactID = dbc.getXactID();
    stmt_name = dbc.getStmtID( query_text, stmt_prefix );
    
    dbc.lock();
    try
    {
	dbc.begin( JDBC_MSG_QUERY );
	dbc.write( JDBC_QRY_PREP );
	dbc.write( JDBC_QP_STMT_NAME );
	dbc.write( stmt_name );
	dbc.write( JDBC_QP_QTXT );
	dbc.write( query_text );
	dbc.done( true );

	rsmd = readResults();
    }
    catch( EdbcEx ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ": error preparing query" );
	if ( trace.enabled( 1 ) )  ex.trace( trace );
	throw ex;
    }
    finally 
    { 
	dbc.unlock(); 
    }

    return;
}


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
*/

private void
exec( ParamSet param_set )
    throws EdbcEx
{
    /*
    ** Is the current driver transaction ID different
    ** than the ID obtained when the query was prepared?
    ** If so, the prepared statement was lost with the
    ** change in transaction state and we need to prepare
    ** the query again.
    */
    clearResults();
    if ( xactID != dbc.getXactID() )  prepare();

    dbc.lock();
    try
    {
	if ( rsmd == null )
	{
	    /*
	    ** Non-query statement (execute).
	    */
	    dbc.begin( JDBC_MSG_QUERY );
	    dbc.write( JDBC_QRY_EXS );
	    dbc.write( JDBC_QP_STMT_NAME );
	    dbc.write( stmt_name );

	    if ( param_set.getCount() <= 0 )
		dbc.done( true );
	    else
	    {
		dbc.done( false );
		send_desc( param_set );
		send_params( param_set );
	    }

	    readResults( timeout );
	}
	else
	{
	    /*
	    ** Select statement (open a cursor).  Generate a 
	    ** cursor ID if not provided by the application.
	    */
	    int	    concurrency = getConcurrency( qry_concur );
	    String  stmt = stmt_name;
	    String  cursor = (crsr_name != null) 
			     ? crsr_name : dbc.getUniqueID( crsr_prefix );

	    dbc.begin( JDBC_MSG_QUERY );
	    dbc.write( JDBC_QRY_OCS );
	    dbc.write( JDBC_QP_CRSR_NAME );
	    dbc.write( cursor );

	    /*
	    ** Parser removes FOR READONLY clause because it isn't
	    ** a part of Ingres SELECT syntax.  Tell server that
	    ** cursor should be READONLY (kludge older protocol
	    ** by appending clause to statement name).
	    */
	    if ( concurrency == EdbcSQL.CONCUR_READONLY )
		if ( dbc.msg_protocol_level < JDBC_MSG_PROTO_2 )
		    stmt += " for readonly";
		else
		{
		    dbc.write( JDBC_QP_FLAGS );
		    dbc.write( (short)2 );
		    dbc.write( JDBC_QF_READONLY );
		}

	    dbc.write( JDBC_QP_STMT_NAME );
	    dbc.write( stmt );

	    if ( param_set.getCount() <= 0 )
		dbc.done( true );
	    else
	    {
		dbc.done( false );
		send_desc( param_set );
		send_params( param_set );
	    }

	    /*
	    ** Check to see if a new result-set descriptor
	    ** has been received.  If so, it may be different
	    ** than the previous descriptor and will be more
	    ** accurate (save only if present).
	    */
	    EdbcRSMD new_rsmd = readResults( timeout );
	    if ( new_rsmd != null )  rsmd = new_rsmd;

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

	    resultSet = new RsltCurs( this, rsmd, stmt_id, cursor, fetchSize );
	}
    }
    catch( EdbcEx ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".execute(): error executing query" );
	if ( trace.enabled( 1 ) )  ex.trace( trace );
	throw ex;
    }
    finally 
    { 
	dbc.unlock(); 
    }

    return;
} // exec


/*
** Name: send_desc
**
** Description:
**	Send a JDBC_MSG_DESC message for the current set
**	of parameters.
**
** Input:
**	param_set	Parameters.
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
**	17-Dec-99 (gordy)
**	    Conversion to BLOB now determined by DBMS max varchar len.
**	 2-Feb-00 (gordy)
**	    Send short streams as VARCHAR or VARBINARY.
**	 7-Feb-00 (gordy)
**	    For the NULL datatype or NULL data values, send the generic
**	    NULL datatype and the nullable indicator.
**	13-Jun-00 (gordy)
**	    Nullable byte converted to flags, parameter names supported
**	    for database procedures (EdbcCall).
**	15-Nov-00 (gordy)
**	    Extracted Param class to create a stand-alone Paramset class.
**	 8-Jan-01 (gordy)
**	    Parameter set passed as parameter to support batch processing.
**	10-May-01 (gordy)
**	    Now pass DBMS type to support alternate formats (UCS2 for now).
**	    Character arrays also supported.
**	 1-Jun-01 (gordy)
**	    Removed conversion of BLOB to fixed length type for short lengths.
**      13-Aug-01 (loera01) SIR 105521
**          Set the JDBC_DSC_GTT flag if the parameter is a global temp
**          table.
**	20-Feb-02 (gordy)
**	    Decimal not supported by gateways at proto level 0.  Send as float.
**	 6-Sep-02 (gordy)
**	    Use formatted length of CHAR and VARCHAR parameters rather than
**	    character length.
*/

protected synchronized void
send_desc( ParamSet param_set )
    throws EdbcEx
{
    int	param_cnt = param_set.getCount();
    dbc.begin( JDBC_MSG_DESC );
    dbc.write( (short)param_cnt );
    
    for( int param = 0; param < param_cnt; param++ )
    {
	short	type = (short)param_set.getType( param );
	short	dbms = 0;
	String	name = param_set.getName( param );
	int	length = 0;
	byte	prec = 0;
	byte	scale = 0;
	byte	flags = 0;

	Object	value = param_set.getValue( param );
	byte	pflags = param_set.getFlags( param );
	boolean	alt = (pflags & ParamSet.PS_FLG_ALT) != 0;

	switch( type )
	{
	case Types.NULL :
	    dbms = DBMS_TYPE_LONG_TEXT;
	    flags |= JDBC_DSC_NULL;
	    break;

	case Types.BIT : 
	    type = (short)Types.TINYINT;
	    dbms = DBMS_TYPE_INT;
	    length = 1;
	    break;
	
	case Types.TINYINT :
	    dbms = DBMS_TYPE_INT;
	    length = 1;
	    break;

	case Types.SMALLINT :
	    dbms = DBMS_TYPE_INT;
	    length = 2;
	    break;

	case Types.INTEGER :
	    dbms = DBMS_TYPE_INT;
	    length = 4;
	    break;

	case Types.REAL :
	    dbms = DBMS_TYPE_FLOAT;
	    length = 4;
	    break;

	case Types.DOUBLE :
	    dbms = DBMS_TYPE_FLOAT;
	    length = 8;
	    break;

	case Types.BIGINT :
	    if ( dbc.is_ingres  ||  dbc.db_protocol_level >= DBMS_PROTO_1 )
	    {
		type = (short)Types.DECIMAL;
		dbms = DBMS_TYPE_DECIMAL;
		prec = 15;
		
		if ( value != null )
		{
		    long	val = ((Long)value).longValue();
		    String	str = ((Long)value).toString();
		    prec = (byte)(str.length() - (val < 0 ? 1 : 0));
		}
	    }
	    else
	    {
		type = (short)Types.DOUBLE;
		dbms = DBMS_TYPE_FLOAT;
		length = 8;
	    }
	    break;

	case Types.DECIMAL :
	    if ( dbc.is_ingres  ||  dbc.db_protocol_level >= DBMS_PROTO_1 )
	    {
		dbms = DBMS_TYPE_DECIMAL;
		prec = 15;

		if ( value != null )
		{
		    BigDecimal  dec = (BigDecimal)value;
		    String	str = dec.toString();

		    prec = (byte)(str.length() - (dec.signum() < 0 ? 1 : 0) 
					       - (dec.scale() > 0 ? 1 : 0));
		    scale = (byte)dec.scale();
		}
	    }
	    else
	    {
		type = (short)Types.DOUBLE;
		dbms = DBMS_TYPE_FLOAT;
		length = 8;
	    }
	    break;
	
	case Types.DATE :
	    type = (short)Types.TIMESTAMP;
	    dbms = DBMS_TYPE_DATE;
	    break;

	case Types.TIME :
	    type = (short)Types.TIMESTAMP;
	    dbms = DBMS_TYPE_DATE;
	    break;

	case Types.TIMESTAMP :
	    dbms = DBMS_TYPE_DATE;
	    break;

	case Types.CHAR :
	    /*
	    ** For UCS2 connections, alt means don't send
	    ** UCS2 data.  For non-UCS2 connections, UCS2
	    ** data is never sent (alt is forced true).
	    */
	    if ( ! dbc.ucs2_supported )  alt = true;
		
	    /*
	    ** Determine the byte length of the string as
	    ** formatted for transmission to the Server.
	    ** Nulls are sent as zero length strings.  
	    */
	    if ( value == null )
	        length = 0;
	    else  if ( alt )
		length = dbc.getByteLength( (char[])value );
	    else
		length = dbc.getUCS2ByteLength( (char[])value );

	    /*
	    ** Long character arrays are sent as BLOBs.
	    ** Zero length arrays must be sent as VARCHAR.
	    ** UCS2 used when supported unless alternate 
	    ** format is requested.
	    */
	    if ( length > 0  &&  length <= max_vchr_len )
		dbms = alt ? DBMS_TYPE_CHAR : DBMS_TYPE_NCHAR;
	    else  if ( length == 0 )
	    {
		type = (short)Types.VARCHAR;
		dbms = alt ? DBMS_TYPE_VARCHAR : DBMS_TYPE_NVARCHAR;
	    }
	    else
	    {
		type = (short)Types.LONGVARCHAR;
		dbms = alt ? DBMS_TYPE_LONG_CHAR : DBMS_TYPE_LONG_NCHAR;
		length = 0;
	    }
	    break;

	case Types.VARCHAR :	
	    /*
	    ** For UCS2 connections, alt means don't send
	    ** UCS2 data.  For non-UCS2 connections, UCS2
	    ** data is never sent (alt is forced true).
	    */
	    if ( ! dbc.ucs2_supported )  alt = true;

	    /*
	    ** Determine the byte length of the string as
	    ** formatted for transmission to the Server.
	    ** Nulls are described as zero length strings.  
	    */
	    if ( value == null )
		length = 0;
	    else  if ( alt )
		length = dbc.getByteLength( (String)value );
	    else
		length = dbc.getUCS2ByteLength( (String)value );

	    /*
	    ** Long strings are sent as BLOBs.  UCS2 used 
	    ** when supported unless alternate format is 
	    ** requested.
	    */
	    if ( length <= max_vchr_len )
		dbms = alt ? DBMS_TYPE_VARCHAR : DBMS_TYPE_NVARCHAR;
	    else
	    {
		type = (short)Types.LONGVARCHAR;
		dbms = alt ? DBMS_TYPE_LONG_CHAR : DBMS_TYPE_LONG_NCHAR;
		length = 0;
	    }
	    break;

	case Types.LONGVARCHAR :
	    /*
	    ** For non-UCS2 connections, alt is ignored.
	    ** For UCS2 connections, alt means don't send
	    ** UCS2 data.  For consistency, we force alt 
	    ** for non-UCS2 connections.
	    */
	    if ( ! dbc.ucs2_supported )  alt = true;
	    dbms = alt ? DBMS_TYPE_LONG_CHAR : DBMS_TYPE_LONG_NCHAR;
	    length = 0;
	    break;

	case Types.BINARY :
	    /*
	    ** Long arrays are sent as BLOBs.
	    */
	    length = (value == null) ? 0 : ((byte[])value).length;

	    if ( length <= max_vchr_len )
		dbms = DBMS_TYPE_BYTE;
	    else
	    {
		type = (short)Types.LONGVARBINARY;
		dbms = DBMS_TYPE_LONG_BYTE;
		length = 0;
	    }
	    break;

	case Types.VARBINARY :
	    /*
	    ** Long arrays are sent as BLOBs.
	    */
	    length = (value == null) ? 0 : ((byte[])value).length;

	    if ( length <= max_vchr_len )
		dbms = DBMS_TYPE_VARBYTE;
	    else
	    {
		type = (short)Types.LONGVARBINARY;
		dbms = DBMS_TYPE_LONG_BYTE;
		length = 0;
	    }
	    break;

	case Types.LONGVARBINARY :
	    dbms = DBMS_TYPE_LONG_BYTE;
	    length = 0;
	    break;
	}

	if ( value == null )  flags |= JDBC_DSC_NULL;

	if ( (pflags & ParamSet.PS_FLG_PROC_PARAM) != 0 )  
	    flags |= JDBC_DSC_PIN;
	
	if ( (pflags & ParamSet.PS_FLG_PROC_BYREF) != 0 )  
	    flags |= JDBC_DSC_POUT;

	if ( (pflags & ParamSet.PS_FLG_PROC_GTT) != 0 )  
	    flags |= JDBC_DSC_GTT;

	dbc.write( (short)type );
	dbc.write( (short)dbms );
	dbc.write( (short)length );
	dbc.write( (byte)prec );
	dbc.write( (byte)scale );
	dbc.write( (byte)flags );
	
	if ( name == null )
	    dbc.write( (short)0 );	// no name.
	else
	    dbc.write( name );
    }

    dbc.done( false );
    return;
} // send_desc


/*
** Name: send_params
**
** Description:
**	Send a JDBC_MSG_DATA message for the current set
**	of parameters.
**
**	VARCHAR and VARBINARY represent all non-stream
**	input strings and byte arrays.  Since a single
**	parameter has fixed length, these are converted
**	to CHAR and BINARY when sent to the server.  If
**	their lengths exceed the max for non-BLOBs, they
**	are converted to LONGVARCHAR or LONGVARBINARY.
**
** Input:
**	param_set	Parameters.
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
**	29-Sep-99 (gordy)
**	    Implemented support for BLOBs.
**	12-Nov-99 (gordy)
**	    Use configured date formatter.
**	17-Dec-99 (gordy)
**	    Conversion to BLOB now determined by DBMS max varchar len.
**	 2-Feb-00 (gordy)
**	    Send short streams as VARCHAR or VARBINARY.
**	15-Nov-00 (gordy)
**	    Extracted Param class to create a stand-alone Paramset class.
**	 8-Jan-01 (gordy)
**	    Parameter set passed as parameter to support batch processing.
**	10-May-01 (gordy)
**	    Support UCS2 as alternate format for character types.  Char
**	    arrays fully supported as separate BINARY and VARBINARY types.
**	 1-Jun-01 (gordy)
**	    Removed conversion of BLOB to fixed length type for short lengths.
**	20-Feb-02 (gordy)
**	    Decimal not supported by gateways at proto level 0.  Send as float.
**	 6-Sep-02 (gordy)
**	    Use formatted length of CHAR and VARCHAR parameters rather than
**	    character length.
*/

protected synchronized void
send_params( ParamSet param_set )
    throws EdbcEx
{
    int param_cnt = param_set.getCount();
    dbc.begin( JDBC_MSG_DATA );
    
    for( int param = 0; param < param_cnt; param++ )
    {
	Object	obj = param_set.getValue( param );
	short	type = (short)param_set.getType( param );
	boolean	alt = (param_set.getFlags(param) & ParamSet.PS_FLG_ALT) != 0;
	int	length;

	if ( type == Types.NULL  || obj == null )
	{
	    dbc.write( (byte)0 );   // NULL
	    continue;
	}

	dbc.write( (byte)1 );   // Not NULL

	switch( type )
	{
	case Types.BIT :	dbc.write( (byte)(((Boolean)obj).booleanValue() 
					   ? 1 : 0) );			break;
	case Types.TINYINT :    dbc.write( ((Byte)obj).byteValue() );   break;
	case Types.SMALLINT :   dbc.write( ((Short)obj).shortValue() ); break;
	case Types.INTEGER :    dbc.write( ((Integer)obj).intValue() ); break;
	case Types.REAL :	dbc.write( ((Float)obj).floatValue() ); break;
	case Types.DOUBLE :	dbc.write(((Double)obj).doubleValue()); break;

	case Types.DATE :
	case Types.TIME :
	case Types.TIMESTAMP :	dbc.write( (String)obj );		break;

	case Types.BIGINT :	
	    if ( dbc.is_ingres  ||  dbc.db_protocol_level >= DBMS_PROTO_1 )
		dbc.write( ((Long)obj).toString() );    
	    else
		dbc.write( ((Long)obj).doubleValue() );
	    break;

	case Types.DECIMAL :    
	    if ( dbc.is_ingres  ||  dbc.db_protocol_level >= DBMS_PROTO_1 )
		dbc.write( ((BigDecimal)obj).toString() );
	    else
		dbc.write( ((BigDecimal)obj).doubleValue() );
	    break;

	case Types.CHAR :
	    /*
	    ** For UCS2 connections, alt means don't send
	    ** UCS2 data.  For non-UCS2 connections, UCS2
	    ** data is never sent (alt is forced true).
	    */
	    if ( ! dbc.ucs2_supported )  alt = true;

	    /*
	    ** Determine the byte length of the string as
	    ** formatted for transmission to the Server.
	    */
	    if ( alt )
		length = dbc.getByteLength( (char[])obj );
	    else
		length = dbc.getUCS2ByteLength( (char[])obj );

	    /*
	    ** Long character arrays are sent as BLOBs.
	    ** Zero length arrays must be sent as VARCHAR.
	    ** UCS2 used when supported unless alternate 
	    ** format is requested.
	    */
	    if ( length > 0  &&  length <= max_vchr_len )
		if ( alt )
		    dbc.write( (char[])obj, 0, ((char[])obj).length );
		else
		    dbc.writeUCS2( (char[])obj, 0, ((char[])obj).length );
	    else  if ( length == 0 )	// Zero length CHAR sent as VARCHAR
		if ( alt )
		    dbc.write( (char[])obj );
		else
		    dbc.writeUCS2( (char[])obj );
	    else  if ( alt )
		dbc.write( new CharArrayReader( (char[])obj ) );
	    else
		dbc.writeUCS2( new CharArrayReader( (char[])obj ) );
	    break;

	case Types.VARCHAR :	
	    /*
	    ** For UCS2 connections, alt means don't send
	    ** UCS2 data.  For non-UCS2 connections, UCS2
	    ** data is never sent (alt is forced true).
	    */
	    if ( ! dbc.ucs2_supported )  alt = true;

	    /*
	    ** Determine the byte length of the string as
	    ** formatted for transmission to the Server.
	    */
	    if ( alt )
		length = dbc.getByteLength( (String)obj );
	    else
		length = dbc.getUCS2ByteLength( (String)obj );

	    /*
	    ** Long strings are sent as BLOBs.  UCS2 used 
	    ** when supported unless alternate format is 
	    ** requested.
	    */
	    if ( length <= max_vchr_len )
		if ( alt )
		    dbc.write( (String)obj );
		else
		    dbc.writeUCS2( (String)obj );
	    else  if ( alt )
		dbc.write( new StringReader( (String)obj ) );
	    else
		dbc.writeUCS2( new StringReader( (String)obj ) );
	    break;

	case Types.LONGVARCHAR :
	    /*
	    ** For non-UCS2 connections, alt is ignored.
	    ** For UCS2 connections, alt means don't send
	    ** UCS2 data.  For consistency, we force alt for 
	    ** non-UCS2 connections.
	    */
	    if ( ! dbc.ucs2_supported )  alt = true;

	    if ( alt )
		dbc.write( (Reader)obj );
	    else
		dbc.writeUCS2( (Reader)obj );
	    break;

	case Types.BINARY :
	    if ( ((byte[])obj).length <= max_vchr_len )
		dbc.write( (byte[])obj, 0, ((byte[])obj).length );
	    else
	        dbc.write( new ByteArrayInputStream( (byte[])obj ) );
	    break;

	case Types.VARBINARY :
	    if ( ((byte[])obj).length <= max_vchr_len )	
		dbc.write( (byte[])obj );
	    else
		dbc.write( new ByteArrayInputStream( (byte[])obj ) );
	    break;

	case Types.LONGVARBINARY :
	    dbc.write( (InputStream)obj );
	    break;
	}
    }

    dbc.done( true );
    return;
} // send_params


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
    if ( trace.enabled() )  trace.log( title + ".getMetaData(): " + rsmd );
    if ( rsmd == null  &&  empty == null )  
	empty = new EdbcRSMD( (short)0, trace );
    return( (rsmd == null) ? empty : rsmd );
} // getMetadata


/*
** Name: clearParameters
**
** Description:
**	Clear stored parameter values and free resources..
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
*/

public synchronized void
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
*/

public void
setNull( int paramIndex, int sqlType, String typeName )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setNull( " + paramIndex + ", " + 
					  sqlType + ", " + typeName + " )" );

    params.setObject( paramMap(paramIndex), null, sqlType, 0 );
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
*/

public void
setBoolean( int paramIndex, boolean value )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setBoolean( " + paramIndex + ", " + value + " )" );

    params.set( paramMap( paramIndex ), Types.BIT, new Boolean( value ) );
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
*/

public void
setByte( int paramIndex, byte value )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setByte( " + paramIndex + ", " + value + " )" );
    
    params.set( paramMap( paramIndex ), Types.TINYINT, new Byte( value ) );
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
*/

public void
setShort( int paramIndex, short value )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setShort( " + paramIndex + ", " + value + " )" );
    
    params.set( paramMap( paramIndex ), Types.SMALLINT, new Short( value ) );
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
*/

public void
setInt( int paramIndex, int value )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + " setInt( " + paramIndex + ", " + value + " )" );

    params.set( paramMap( paramIndex ), Types.INTEGER, new Integer( value ) );
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
*/

public void
setLong( int paramIndex, long value )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setLong( " + paramIndex + ", " + value + " )" );

    params.set( paramMap( paramIndex ), Types.BIGINT, new Long( value ) );
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
*/

public void
setFloat( int paramIndex, float value )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setFloat( " + paramIndex + ", " + value + " )" );

    params.set( paramMap( paramIndex ), Types.REAL, new Float( value ) );
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
*/

public void
setDouble( int paramIndex, double value )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setDouble( " + paramIndex + ", " + value + " )" );

    params.set( paramMap( paramIndex ), Types.DOUBLE, new Double( value ) );
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
**	15-Feb-02 (gordy)
**	    A null parameter value should be mapped to a SQL NULL.
*/

public void
setBigDecimal( int paramIndex, BigDecimal value )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setBigDecimal( " + paramIndex + ", " + value + " )" );

    if ( value == null )
	params.setObject( paramMap( paramIndex ), null, Types.DECIMAL, 0 );
    else
	params.set( paramMap( paramIndex ), Types.DECIMAL, value );
    
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
**	15-Feb-02 (gordy)
**	    A null parameter value should be mapped to a SQL NULL.
*/

public void
setString( int paramIndex, String value )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setString( " + paramIndex + ", " + value + " )" );

    if ( value == null )
	params.setObject( paramMap( paramIndex ), null, Types.VARCHAR, 0 );
    else
	params.set( paramMap( paramIndex ), Types.VARCHAR, value );
    
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
*/

public void
setBytes( int paramIndex, byte value[] )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setBytes( " + paramIndex + ", " + value + " )" );

    params.set( paramMap( paramIndex ), Types.VARBINARY, value );
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
**	15-Feb-02 (gordy)
**	    A null parameter value should be mapped to a SQL NULL.
*/

public void
setDate( int paramIndex, Date value, Calendar cal )
    throws SQLException
{
    String str;

    if ( trace.enabled() )  
	trace.log( title + ".setDate( " + paramIndex + ", " + value + " )" );

    if ( value == null )
	params.setObject( paramMap( paramIndex ), null, Types.DATE, 0 );
    else
    {
	if ( cal == null )
	    str = value.toString();
	else  synchronized( df_date )
	{
	    df_date.setTimeZone( cal.getTimeZone() );
	    str = df_date.format( (java.util.Date)value );
	}
        ParamSet.validateDateTimeStr( str );
	params.set( paramMap(paramIndex), Types.DATE, str );
    }

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
**	15-Feb-02 (gordy)
**	    A null parameter value should be mapped to a SQL NULL.
*/

public void
setTime( int paramIndex, Time value, Calendar cal )
    throws SQLException
{
    String  str;

    if ( trace.enabled() )  
	trace.log( title + ".setTime( " + paramIndex + ", " + value + " )" );

    if ( value == null )
	params.setObject( paramMap( paramIndex ), null, Types.TIME, 0 );
    else
    {
	if ( dbc.use_gmt_tz  ||  cal == null )
	    str = formatConnDate( (java.util.Date)value );
	else  synchronized( df_ts )
	{
	    df_ts.setTimeZone( cal.getTimeZone() );
	    str = df_ts.format( (java.util.Date)value );
	}
        ParamSet.validateDateTimeStr( str );
	params.set( paramMap(paramIndex), Types.TIME, str );
    }

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
**	15-Feb-02 (gordy)
**	    A null parameter value should be mapped to a SQL NULL.
**      10-Feb-04 (rajus01) Bug #111769 Startrack #EDJDBC82
**     	    An invalid Timestamp value causes the JDBC server abort the
**          client connection. Validate the Timestamp string before 
**          passing it to JDBC server.
*/

public void
setTimestamp( int paramIndex, Timestamp value, Calendar cal )
    throws SQLException
{
    String  str;

    if ( trace.enabled() )  
	trace.log( title + ".setTimestamp( " + paramIndex + ", " + value + " )" );

    if ( value == null )
	params.setObject( paramMap( paramIndex ), null, Types.TIMESTAMP, 0 );
    else
    {
	if ( dbc.use_gmt_tz  ||  cal == null )
	    str = formatConnDate( (java.util.Date)value );
	else  synchronized( df_ts )
	{
	    df_ts.setTimeZone( cal.getTimeZone() );
	    str = df_ts.format( (java.util.Date)value );
	}

        ParamSet.validateDateTimeStr( str );
	params.set( paramMap( paramIndex ), Types.TIMESTAMP, str );
    }

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
**	15-Feb-02 (gordy)
**	    A null parameter value should be mapped to a SQL NULL.
*/

public void
setBinaryStream( int paramIndex, InputStream stream, int length )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setBinaryStream( " + paramIndex + ", " + length + " )" );

    if ( stream == null )
	params.setObject( paramMap(paramIndex), null, Types.LONGVARBINARY, 0 );
    else
	params.set( paramMap( paramIndex ), Types.LONGVARBINARY, stream );
    
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
**	15-Feb-02 (gordy)
**	    A null parameter value should be mapped to a SQL NULL.
*/

public void
setAsciiStream( int paramIndex, InputStream stream, int length )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setAsciiStream( " + paramIndex + ", " + length + " )" );

    if ( stream == null )
	params.setObject( paramMap( paramIndex ), null, Types.LONGVARCHAR, 0 );
    else
	params.set( paramMap( paramIndex ), 
		    Types.LONGVARCHAR, new InputStreamReader( stream ) );
    
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
**	15-Feb-02 (gordy)
**	    A null parameter value should be mapped to a SQL NULL.
*/

public void
setUnicodeStream( int paramIndex, InputStream stream, int length )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setUnicodeStream( " + paramIndex + ", " + length + " )" );

    if ( stream == null )
	params.setObject( paramMap( paramIndex ), null, Types.LONGVARCHAR, 0 );
    else
    {
	try
	{
	    params.set( paramMap( paramIndex ), Types.LONGVARCHAR, 
			new InputStreamReader( stream, "UTF-8" ) );
	}
	catch( UnsupportedEncodingException ex )
	{
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR ); 
	}
    }

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
**	15-Feb-02 (gordy)
**	    A null parameter value should be mapped to a SQL NULL.
*/

public void
setCharacterStream( int paramIndex, Reader reader, int length )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setCharacterStream( " + paramIndex + ", " + 
						     length + " )" );
    if ( reader == null )
	params.setObject( paramMap( paramIndex ), null, Types.LONGVARCHAR, 0 );
    else
	params.set( paramMap( paramIndex ), Types.LONGVARCHAR, reader );
    
    return;
} // setCharacterStream


/*
** Name: setArray
**
** Description:
**	Set parameter to an array.
**
** Input:
**	paramIndex	Parameter index.
**	array		Array
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-00 (gordy)
**	    Created.
*/

public void
setArray( int paramIndex, Array array )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setArray( " + paramIndex + " )" );
    throw EdbcEx.get( E_JD0012_UNSUPPORTED );
} // setArray


/*
** Name: setBlob
**
** Description:
**	Set parameter to a Blob.
**
** Input:
**	paramIndex	Parameter index.
**	blob		Blob
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-00 (gordy)
**	    Created.
*/

public void
setBlob( int paramIndex, Blob blob )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setBlob( " + paramIndex + " )" );
    throw EdbcEx.get( E_JD0012_UNSUPPORTED );
} // setBlob


/*
** Name: setClob
**
** Description:
**	Set parameter to a Clob.
**
** Input:
**	paramIndex	Parameter index.
**	clob		Clob
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-00 (gordy)
**	    Created.
*/

public void
setClob( int paramIndex, Clob clob )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setClob( " + paramIndex + " )" );
    throw EdbcEx.get( E_JD0012_UNSUPPORTED );
} // setClob


/*
** Name: setRef
**
** Description:
**	Set parameter to a Ref.
**
** Input:
**	paramIndex	Parameter index.
**	ref		Ref
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-00 (gordy)
**	    Created.
*/

public void
setRef( int paramIndex, Ref ref )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setRef( " + paramIndex + " )" );
    throw EdbcEx.get( E_JD0012_UNSUPPORTED );
} // setRef


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
**	index		Parameter index returned by paramMap().
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
*/

public void
setObject( int paramIndex, Object value, int sqlType, int scale )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setObject( " + 
		   paramIndex + ", " + sqlType + ", " + scale + " )" );
    
    paramIndex = paramMap( paramIndex );

    if ( sqlType != Types.OTHER )
	params.setObject( paramIndex, value, sqlType, scale );
    else
    {
	params.setObject(paramIndex, value, params.getSqlType(value), scale);
	params.setFlags( paramIndex, ParamSet.PS_FLG_ALT );
    }
    return;
}


/*
** Data save methods which are simple covers 
** for the primary data save methods.
*/

public void setNull( int paramIndex, int sqlType ) throws SQLException
{ setNull( paramIndex, sqlType, null ); return; }

public void setDate( int paramIndex, Date value ) throws SQLException
{ setDate( paramIndex, value, null ); return; }

public void setTime( int paramIndex, Time value ) throws SQLException
{ setTime( paramIndex, value, null ); return; }

public void setTimestamp( int paramIndex, Timestamp value ) throws SQLException
{ setTimestamp( paramIndex, value, null ); return; }

public void 
setObject( int paramIndex, Object value, int sqlType ) 
    throws SQLException
{
    setObject( paramIndex, value, sqlType, 0 );
    return;
}

public void
setObject( int paramIndex, Object value )
    throws SQLException
{
    setObject( paramIndex, value, params.getSqlType( value ), -1 );
    return;
}


/*
** Name: readDesc
**
** Description:
**	Read a query result data descriptor message.  Overrides the
**	method in EdbcStmt.  Maintains a single RSMD.  Subsequent
**	descriptor messages are loaded into the existing RSMD.
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
**	16-Apr-01 (gordy)
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

protected synchronized int
paramMap( int index )
    throws EdbcEx
{
    if ( index < 1 )  throw EdbcEx.get( E_JD0007_INDEX_RANGE );
    return( index - 1 );
} // paramMap

} // class EdbcPrep
