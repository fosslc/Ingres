/*
** Copyright (c) 2002, 2009 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.Collections;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: advanstmt.cs
	**
	** Description:
	**	Defines class which implements the Statement interface.
	**
	**  Classes:
	**
	**	AdvanStmt
	**
	** History:
	**	13-May-99 (gordy)
	**	    Created.
	**	 8-Sep-99 (gordy)
	**	    Created new base class for class which interact with
	**	    the server and extracted common data and methods.
	**	    Synchronize entire request/response with server.
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
	**	    Added support for 2.0 extensions.
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
	**	    Pass cursor name to result-set according to spec.
	**	21-Jun-01 (gordy)
	**	    Reworked parsing to remove redundant code, transform
	**	    'SELECT FOR UPDATE' and flag 'FOR UPDATE'.
	**	20-Aug-01 (gordy)
	**	    Support UPDATE concurrency.  Send READONLY query flag to server.
	**	25-Oct-01 (gordy)
	**	    Select loops are only supported at PROTO_2 and above.
	**	26-Aug-02 (thoda04)
	**	    Ported for .NET Provider.
	**	31-Oct-02 (gordy)
	**	    Adapted for generic GCF driver.
	**	19-Feb-03 (gordy)
	**	    Updated to 3.0.
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
	**	25-Feb-04 (thoda04)
	**	    Convert SqlEx exception to an IngresException.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	*/



	/*
	** Name: AdvanStmt
	**
	** Description:
	**	driver class which implements the Statement interface.
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
	**
	**  Private Methods:
	**
	**	exec			Execute a statement.
	**	enableTimer		Start timer for query timeout.
	**	disableTimer		Disable query timer.
	**
	** History:
	**	13-May-99 (gordy)
	**	    Created.
	**	 8-Sep-99 (gordy)
	**	    Created new base class for class which interact with
	**	    the server and extracted common data and methods.
	**	    Synchronize entire request/response with server.
	**	26-Oct-99 (gordy)
	**	    Combined bulk of executeQuery(), executeUpdate() and
	**	    execute() code into single internal method.
	**	16-Nov-99 (gordy)
	**	    Added query timeouts: timer, timed_out, timeExpired().
	**	 4-Oct-00 (gordy)
	**	    Added instance count, inst_count, and instance ID,
	**	    inst_id, for standardized internal tracing.
	**	31-Oct-00 (gordy)
	**	    Support 2.0 extensions.  Added conn, rs_type, rs_concur,
	**	    rs_fetch_dir, rs_fetch_size, getConnection(), getResultSetType(), 
	**	    getResultSetConcurrency(), getFetchDirection(), getFetchSize(), 
	**	    setFetchDirection(), setFetchSize(), addBatch(), clearBatch(), 
	**	    executeBatch().  Renamed max_row_cnt to rs_max_rows and 
	**	    max_col_len to rs_max_len.
	**	 5-Jan-01 (gordy)
	**	    Added support for batch processing.  Added batch.
	**	23-Jan-01 (gordy)
	**	    The batch field cannot be allocated until a batch request is
	**	    made so as to remain compatible.
	**	23-Feb-01 (gordy)
	**	    Removed finalize() method.  Associated result-set does not need 
	**	    to be explicitly closed when this statement is no longer used.  
	**	    The result-set may still be in use, even if the statement is no 
	**	    longer referenced.  The AdvanRslt class has it's own finalize() 
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
	**	    Updated to 3.0.  Added rs_hold, getResultSetHoldability(),
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
	*/

	class
		AdvanStmt : DrvObj, Timer.ICallback
	{
		/*
		** The following require package access so that result-sets can
		** access the info without triggering tracing or worrying about 
		** exceptions.
		*/
		internal int	rs_fetch_dir = FETCH_FORWARD;
		internal int	rs_fetch_size = 0;
		internal int	rs_max_rows = 0;
		internal int	rs_max_len = 0;

		/*
		** Constants
		*/
		protected const int	QUERY = 1;
		protected const int	UPDATE = 0;
		protected const int	UNKNOWN = -1;

		protected const String   crsr_prefix = "DOTNET_CRSR_";
		protected const String   stmt_prefix = "DOTNET_STMT_";

		protected ArrayList	batch = null;
		protected AdvanRslt		resultSet = null;
		protected String		crsr_name = null;
		protected int		rs_type;
		protected int		rs_concur;
		protected int		rs_hold;
		protected bool		parse_escapes = true;
		protected int		timeout = 0;

		/*
		** Result-sets for returning auto generated keys.
		*/
		/*
		private static AdvanRSMD	rsmdKeys = null;
		private static AdvanRSMD	rsmdTblKey = null;
		private static AdvanRSMD	rsmdObjKey = null;
		private static AdvanRSMD	rsmdEmpty = null;
		private static AdvanRslt	rsEmpty = null;
		*/

		/*
		** The only thing we can do to timeout a query is
		** set a timer locally and attempt to cancel the
		** query if the time expires.  We also flag the
		** timeout condition to translate the cancelled
		** query error into a timeout error.
		*/
		private Timer		timer = null;
		private bool		timed_out = false;


		/*
		** Name: AdvanStmt
		**
		** Description:
		**	Class constructor.
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
		**	    Changed parameters for 2.0 extensions.
		**	28-Mar-01 (gordy)
		**	    Connection fields now have package access instead of methods.
		**	31-Oct-02 (gordy)
		**	    Adapted for generic GCF driver.
		**	19-Feb-03 (gordy)
		**	    Added default ResultSet holdability.
		*/

		public
			AdvanStmt( DrvConn conn, int rs_type, int rs_concur, int rs_hold ) : base( conn )
		{
			this.rs_type = rs_type;
			this.rs_concur = rs_concur;
			this.rs_hold = rs_hold;
			title = trace.getTraceName() + "-Statement[" + inst_id + "]";
			tr_id = "Stmt[" + inst_id + "]";
			return;
		} // AdvanStmt


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

		protected internal virtual AdvanRslt
			executeQuery( String sql )
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
		*/

		protected internal virtual int
			executeUpdate( String sql )
		{
			if ( trace.enabled() )  trace.log(title + ".executeUpdate('" + sql + "')");
			exec( sql, UPDATE );
			int count = rslt_rowcount ? rslt_val_rowcnt : 0;
			if ( trace.enabled() )  trace.log( title + ".executeUpdate(): " + count );
			return( count );
		} // executeUpdate

#if _NOT_NEEDED
		public int
			executeUpdate( String sql, int autoGeneratedKeys )
		{
			if ( trace.enabled() )  trace.log( title + ".executeUpdate('" + 
										sql + "'," + autoGeneratedKeys + ")" );
			switch( autoGeneratedKeys )
			{
				case RETURN_GENERATED_KEYS :    break;
				case NO_GENERATED_KEYS :	    break;
				default : throw SqlEx.get( ERR_GC4010_PARAM_VALUE );
			}

			exec( sql, UPDATE );
			int count = rslt_rowcount ? rslt_val_rowcnt : 0;
			if ( trace.enabled() )  trace.log( title + ".executeUpdate(): " + count );
			return( count );
		} // executeUpdate
#endif

		public int
			executeUpdate( String sql, int[] columnIndexes )
		{
			if ( trace.enabled() )  trace.log( title + ".executeUpdate('" + 
										sql + "'," + columnIndexes + ")" );
			exec( sql, UPDATE );
			int count = rslt_rowcount ? rslt_val_rowcnt : 0;
			if ( trace.enabled() )  trace.log( title + ".executeUpdate(): " + count );
			return( count );
		} // executeUpdate


		public int
			executeUpdate( String sql, String[] columnNames )
		{
			if ( trace.enabled() )  trace.log( title + ".executeUpdate('" + 
										sql + "'," + columnNames + ")" );
			exec( sql, UPDATE );
			int count = rslt_rowcount ? rslt_val_rowcnt : 0;
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
		*/

		protected internal virtual bool 
			execute( String sql )
		{
			if ( trace.enabled() )
				trace.log( title + ".execute( '" + sql + "' )" );

			exec( sql, UNKNOWN );
			bool isRS = (resultSet != null);

			if ( trace.enabled() )
				trace.log( title + ".execute(): " + isRS );
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

		internal AdvanRslt
			getResultSet()
		{
			if ( trace.enabled() )  trace.log(title + ".getResultSet(): " + resultSet);
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
		{
			int count = (resultSet != null  ||  ! rslt_rowcount) ? 
				-1 : rslt_val_rowcnt;
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

		public bool
			getMoreResults()
		{
			if ( trace.enabled() )  trace.log( title + ".getMoreResults(): " + false );
			clearQueryResults( true );
			return( false );
		} // getMoreResults


#if _NOT_NEEDED
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

		public bool
			getMoreResults( int current )
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
					throw SqlEx.get( ERR_GC4010_PARAM_VALUE );
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

		public AdvanRslt
			getGeneratedKeys()
		{
			lock(this)
			{
				ResultSet	rs = rsEmpty;
				SqlData[][]	data;
				SqlByte	bytes;
 
				if ( rslt_tblkey )
					if ( rslt_objkey )
					{
						if ( rsmdKeys == null )
						{
							rsmdKeys = new AdvanRSMD( (short)2, trace );
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
							rsmdTblKey = new AdvanRSMD( (short)1, trace );
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
				else  if ( rslt_objkey )
				{
					if ( rsmdObjKey == null )
					{
						rsmdObjKey = new AdvanRSMD( (short)1, trace );
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
						rsmdEmpty = new AdvanRSMD( (short)1, trace );
						rsmdEmpty.setColumnInfo( "no_key", 1, (int)Types.BINARY, 
							(short)DBMS_TYPE_TBLKEY, 
							(short)MSG_RPV_TBLKEY_LEN, 
							(byte)0, (byte)0, false );
					}

					rs = rsEmpty = new RsltData( conn, rsmdEmpty, null );
				}

				if ( trace.enabled() )  trace.log( title + ".getGeneratedKeys: " + rs );
				return( rs );
			}
		} // getGeneratedKeys
#endif


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

		protected internal override void
			clearResults()
		{
			clearQueryResults( true );
			base.clearResults();
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

		protected void
			clearQueryResults( bool close )
		{
			lock(this)
			{
				if ( close  &&  resultSet != null )
				{
					try { resultSet.shut(); }
					catch( SqlEx ) {}
					resultSet = null;
				}

				rslt_rowcount = false;
				rslt_val_rowcnt = 0;
				return;
			}
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
		**	31-Oct-02 (gordy)
		**	    Batch list allocation now done with newBatch().
		*/

		public void
			addBatch( String sql )
		{
			if ( trace.enabled() )  trace.log( title + ".addBatch( '" + sql + "' )" );
			if ( batch == null )  newBatch();
			lock( batch ) { batch.Add( sql ); }
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
		{
			if ( trace.enabled() )  trace.log( title + ".clearBatch()" );
			if ( batch != null )  lock( batch ) { batch.Clear(); }
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
		**	31-Oct-02 (gordy)
		**	    Check for NULL batch instead of allocating.
		**	19-Feb-03 (gordy)
		**	    Use SUCCESS_NO_INFO result rather than integer literal.
		*/

		protected internal virtual int[]
			executeBatch()
		{
			int[] results;

			if ( trace.enabled() )  trace.log( title + ".executeBatch()" );
			if ( batch == null )  return( new int[0] );

			lock( batch )
			{
				int count = batch.Count;
				results = new int[ count ];

				/*
				** Execute each individual query in the batch.
				*/
				for( int i = 0; i < count; i++ )
					try
					{
						String sql = (String)batch[0];
						batch.RemoveAt(0);
						if ( trace.enabled() )  
							trace.log( title + ".executeBatch[" + i + "]: '" 
								+ sql + "'" );

						exec( sql, UPDATE );
						results[ i ] = rslt_rowcount ? rslt_val_rowcnt 
							: SUCCESS_NO_INFO;

						if ( trace.enabled() )  
							trace.log( title + ".executeBatch[" + i + "] = " 
								+ results[ i ] );
					}
					catch( SqlEx )
					{
						/*
						** Return only the successful query results.
						*/
						int[] temp = new int[ i ];
						if ( i > 0 )  Array.Copy( results, 0, temp, 0, i );

						batch.Clear();
						throw;
					}
					catch( Exception )	// Shouldn't happen
					{
						/*
						** Return only the successful query results.
						*/
						int[] temp = new int[ i ];
						if ( i > 0 )  Array.Copy( results, 0, temp, 0, i );
						results = temp;
						break;
					}
	
				batch.Clear();
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
		**	    Pass cursor name to result-set according to spec:
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
		**	    Timezone now passed to AdvanSQL.
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
		**	19-Jun-06 (gordy)
		**	    I give up... GMT indicator replaced with connection.
		*/

		private void
			exec( String text, int mode )
		{
			SqlParse	parser = new SqlParse( text, conn );
			int		type = parser.getQueryType();
			AdvanRSMD	rsmd;
			String	cursor = null;

			clearResults();

			if ( mode == UNKNOWN )  
				mode = (type == SqlParse.QT_SELECT) ? QUERY : UPDATE;

			if ( type == SqlParse.QT_DELETE  ||  type == SqlParse.QT_UPDATE )
				cursor = parser.getCursorName();

			msg.LockConnection();
			try
			{	
				String	sql = parser.parseSQL( parse_escapes );

				if ( mode == QUERY )
				{
					int	concurrency = getConcurrency( parser.getConcurrency() );

					if ( 
						conn.select_loops  &&  crsr_name == null  &&  
						concurrency != DrvConst.DRV_CRSR_UPDATE  &&
						conn.msg_protocol_level >= MSG_PROTO_2
						)
					{
						/*
						** Select statement using a select loop.
						*/
						bool needEOG = true;
						msg.begin( MSG_QUERY );
						msg.write( MSG_QRY_EXQ );
		
						if ( conn.msg_protocol_level >= MSG_PROTO_3 )
						{
							msg.write( MSG_QP_FLAGS );
							msg.write( (short)4 );
							msg.write( MSG_QF_FETCH_FIRST | MSG_QF_AUTO_CLOSE );
							needEOG = false;
						}
		
						msg.write( MSG_QP_QTXT );
						msg.write( sql );
						msg.done( true );

						if ( (rsmd = readResults( timeout, needEOG )) == null )
							throw SqlEx.get( ERR_GC4017_NO_RESULT_SET );

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
						bool	needEOG = true;
						cursor = (crsr_name != null) ? crsr_name 
							: conn.getUniqueID( crsr_prefix );


						/*
						** Parser removes FOR READONLY clause because it isn't
						** a part of Ingres SELECT syntax.  Tell server that
						** cursor should be READONLY (kludge older protocol
						** by restoring clause to query).
						*/
						if ( concurrency == DrvConst.DRV_CRSR_READONLY )
							if ( conn.msg_protocol_level < MSG_PROTO_2 )
								sql += " for readonly";
							else  if ( conn.msg_protocol_level < MSG_PROTO_3 )
								flags |= MSG_QF_READONLY;
							else
							{
								flags |= MSG_QF_READONLY | MSG_QF_FETCH_FIRST;
								needEOG = false;
							}

						if ( conn.msg_protocol_level >= MSG_PROTO_3 )
							flags |= MSG_QF_AUTO_CLOSE;
			
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
						msg.write( MSG_QP_QTXT );
						msg.write( sql );
						msg.done( true );

						if ( (rsmd = readResults( timeout, needEOG )) == null )
							throw SqlEx.get( ERR_GC4017_NO_RESULT_SET );

						/*
						** The cursor name is passed to the result-set 
						** for updatable cursors or if provided by the 
						** application (2.1 API spec).
						*/
						if ( (rslt_flags & MSG_RF_READ_ONLY) == 0 )  
						{
							resultSet = new RsltUpd( conn, this, 
								rsmd, rslt_val_stmt, cursor );
							if ( msg.moreMessages() )  readResults( timeout, needEOG );
						}
						else
						{
							if ( rs_concur == DrvConst.DRV_CRSR_UPDATE )
								setWarning( SqlEx.get( ERR_GC4016_RS_CHANGED ) );

							resultSet = new RsltCurs( conn, this, rsmd, rslt_val_stmt, 
								crsr_name, getPreFetchSize(),
								msg.moreMessages() );
						}

						msg.UnlockConnection(); 
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
					msg.write( MSG_QP_QTXT );
					msg.write( sql );
					msg.done( true );

					if ( readResults( timeout, true ) != null )	// shouldn't happen
						throw SqlEx.get( ERR_GC4018_RESULT_SET_NOT_PERMITTED );
					msg.UnlockConnection(); 
				}
				else
				{
					/*
					** Non-query statement.
					*/
					msg.begin( MSG_QUERY );
					msg.write( MSG_QRY_EXQ );
					msg.write( MSG_QP_QTXT );
					msg.write( sql );
					msg.done( true );

					if ( (rsmd = readResults( timeout, true )) == null )
						msg.UnlockConnection();		// We be done.
					else
					{
						/*
						** Query unexpectedly returned a result-set.
						** We need to cleanup the tuple stream and
						** statement in the server.  The easiest
						** way to do this is go ahead and create a
						** result-set and close it.
						*/
						resultSet = new RsltSlct( conn, this, rsmd, 
							rslt_val_stmt, 1, false );
						try { resultSet.shut(); }
						catch( SqlEx ) {}
						resultSet = null;
						throw SqlEx.get( ERR_GC4018_RESULT_SET_NOT_PERMITTED );
					}
				}
			}
			catch( SqlEx ex )
			{
				if ( trace.enabled() )  
					trace.log( title + ".execute(): error executing query" );
				if ( trace.enabled( 1 ) )  ex.trace( trace );
				msg.UnlockConnection(); 
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

		public virtual void
			close()
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

		public AdvanConnect
			getConnection()
		{
			if ( trace.enabled() )  
				trace.log( title + ".getConnection(): " + conn.advanConn );
			return( conn.advanConn );
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

//		public int
//			getResultSetConcurrency()
//		{
//			int	concurrency;
//    
//			if ( rs_concur == DRV_CRSR_UPDATE )  
//				concurrency = ResultSet.CONCUR_UPDATABLE;
//			else
//				concurrency = ResultSet.CONCUR_READ_ONLY;   // default
//
//			if ( trace.enabled() )  
//				trace.log( title + ".getResultSetConcurrency(): " + concurrency );
//			return( concurrency );
//		} // getResultSetConcurrency


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

//		public int
//			getResultSetHoldability()
//		{
//			if ( trace.enabled() )  
//				trace.log( title + ".getResultSetHoldability(): " + rs_hold );
//			return( rs_hold );
//		} // getResultSetHoldability


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
			setEscapeProcessing( bool enable )
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

//		public void
//			setFetchDirection( int dir )
//		{
//			if ( trace.enabled() )  
//				trace.log( title + ".setFetchDirection( " + dir + " )" );
//
//			switch( dir )
//			{
//				case ResultSet.FETCH_FORWARD :
//				case ResultSet.FETCH_REVERSE :
//				case ResultSet.FETCH_UNKNOWN :
//					break;
//
//				default :
//					throw SqlEx.get( ERR_GC4010_PARAM_VALUE );
//			}
//
//			rs_fetch_dir = dir;
//			return;
//		} // setFetchDirection


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
		{
			int size = (rs_fetch_size > 0) ? rs_fetch_size : 0;
			if ( trace.enabled() )  trace.log( title + ".getFetchSize(): " + size );
			return( size );
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
		{
			if ( trace.enabled() )  
				trace.log( title + ".setFetchSize( " + rows + " )" );

			if ( rows < 0  ||  (rs_max_rows > 0  &&  rows > rs_max_rows) )
				throw SqlEx.get( ERR_GC4010_PARAM_VALUE );

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
		{
			if ( trace.enabled() )  trace.log( title + ".setMaxRows( " + max + " )" );
			if ( max < 0 )  throw SqlEx.get( ERR_GC4010_PARAM_VALUE );
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
		{
			if ( trace.enabled() )  
				trace.log( title + ".setMaxFieldSize( " + max + " )" );
			if ( max < 0 )  throw SqlEx.get( ERR_GC4010_PARAM_VALUE );
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
		{
			if ( trace.enabled() )  
				trace.log( title + ".setQueryTimeout( " + seconds + " )" );
			if ( seconds < 0 )  throw SqlEx.get( ERR_GC4010_PARAM_VALUE );
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

		protected void
			newBatch()
		{
			lock(this)
			{
				if ( batch == null )  batch = new ArrayList();	// Allocate only once.
			}
			return;
		}


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
			if ( concurrency != DrvConst.DRV_CRSR_DBMS )
				return( concurrency );
			if ( rs_concur   != DrvConst.DRV_CRSR_DBMS )
				return( rs_concur );
			return( conn.readOnly ?
				DrvConst.DRV_CRSR_READONLY : conn.cursor_mode );
		} // getConcurrency


		/*
		** Name: getPreFetchSize
		**
		** Description:
		**	Returns the cursor pre-fetch row limit for the current
		**	result-set based on server suggested fetch size and 
		**	application requested fetch size.
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
		*/

		protected int
			getPreFetchSize()
		{
			/*
			** Pre-fetching is only allowed if the server returned
			** a suggested pre-fetch size, otherwise force singe row.  
			** Application selected size takes precedence over server
			** suggestion.
			*/
			return( (! rslt_prefetch) ? 1 :
				(rs_fetch_size > 0) ? rs_fetch_size : 
				(rslt_val_fetch > 0) ? rslt_val_fetch : 1 );
		} // getPreFetchSize


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
		**	AdvanRSMD	Result-set Meta-data from DESC message.
		**
		** History:
		**	31-Oct-02 (gordy)
		**	    Created.
		**	 6-Oct-03 (gordy)
		**	    Added needEOG parameter to allow RESULT message interrupts.
		*/

		protected AdvanRSMD
			readResults( int timeout, bool needEOG )
		{
			AdvanRSMD rsmd;
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
		**	AdvanRSMD	Result-set Meta-data from DESC message.
		**
		** History:
		**	 6-Oct-03 (gordy)
		**	    Created.
		*/

		protected AdvanRSMD
			readResults( bool needEOG )
		{
			AdvanRSMD rsmd = null;
    
			/*
			** Read results until interrupted or end-of-group.
			*/
			do
			{
				/*
				** There should only be one descriptor,
				** but just to be safe, return last.
				*/
				AdvanRSMD current = readResults(); 
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
		**	AdvanRSMD    Query result data descriptor.
		**
		** History:
		**	 8-Sep-99 (gordy)
		**	    Created.
		*/

		protected internal override AdvanRSMD
			readDesc()
		{
			disableTimer();
			return( AdvanRSMD.load( conn ) );
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

		protected internal AdvanRSMD
		readDesc(AdvanRSMD rsmd)
		{
			disableTimer();

			if (rsmd == null)
				rsmd = AdvanRSMD.load(conn);
			else
				rsmd.reload(conn);

			return (rsmd);
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
		**	SqlEx	Error or NULL.
		**
		** History:
		**	31-Oct-02 (gordy)
		**	    Created.
		*/

		protected internal override SqlEx
			readError()
		{
			SqlEx   ex;
    
			disableTimer();
			ex = base.readError();

			/*
			** We translate the query canceled error from a timeout 
			** into a driver error.
			*/
			if ( timed_out  &&  ex != null  &&
				ex.getErrorCode() == E_AP0009_QUERY_CANCELLED ) 
				ex = SqlEx.get( ERR_GC4006_TIMEOUT );

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

		protected internal override bool
			readResult()
		{
			disableTimer();
			base.readResult();
			return( true );
		} // readDone


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
				timer = new Timer( timeout, (Timer.ICallback)this );
				timer.Start();
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
			catch( Exception ) {}
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
				timer.Interrupt();
				timer = null;
			}
			return;
		} // disableTimer


	} // class AdvanStmt

}  // namespace
