/*
** Copyright (c) 1999, 2010 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.Collections;
using System.IO;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: advanprep.cs
	**
	** Description:
	**	Defines class which implements the PreparedStatement interface.
	**
	**  Classes:
	**
	**	AdvanPrep
	**
	** History:
	**	19-May-99 (gordy)
	**	    Created.
	**	 8-Sep-99 (gordy)
	**	    Created new base class for class which interact with
	**	    the server and extracted common data and methods.
	**	    Synchronize entire request/response with server.
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
	**	    Added support for derived classes (advanCall specfically).
	**	 4-Oct-00 (gordy)
	**	    Standardized internal tracing.
	**	19-Oct-00 (gordy)
	**	    Changes in transaction state result in prepared statements
	**	    being lost.  The transaction state is now tracked and the
	**	    query re-prepared when the state changes.
	**	 1-Nov-00 (gordy)
	**	    Added support for 2.0 extensions.  Added ChrRdr class.
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
	**	    lengths.  requires long fixed length to BLOB conversion,
	**	    but not the other way around.  Since BLOBs can only be
	**	    specified explicitly, we now send in format requested.
	**	13-Jun-01 (gordy)
	**	    2.1 spec requires UTF-8 encoding for unicode streams.
	**	20-Jun-01 (gordy)
	**	    Pass cursor name to result-set according to spec.
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
	**	    Updated to 3.0.
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
	**	25-Feb-04 (thoda04)
	**	    Convert SqlEx exception to an IngresException.
	**	15-Mar-04 (gordy)
	**	    Implemented BIGINT support, default to DECIMAL when not supported.
	**	    BOOLEAN now implemented as always using alternate storage format.
	**	12-May-04 (thoda04)
	**	    For blobs, use write(StreamReader), not write(Reader). 	
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	13-Oct-05 (thoda04) B115418
	**	    CommandBehavior.SchemaOnly should not call database and
	**	    needs to return an empty RSLT and non-empty RSMD metadata.
	**	    Added getResultSetEmpty() method.
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
	**	    Support empty date strings.
	**	 3-Mar-10 (thoda04)  SIR 123368
	**	    Added support for IngresType.IngresDate parameter data type.
	*/



	/*
	** Name: AdvanPrep
	**
	** Description:
	**	driver class which implements the PreparedStatement 
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
	**	setDecimal	    Set a Decimal parameter value.
	**	setString	    Set a String parameter value.
	**	setBytes	    Set a byte array parameter value.
	**	setDate		    Set a Date parameter value.
	**	setTime		    Set a Time parameter value.
	**	setTimestamp	    Set a Timestamp parameter value.
	**	setBinaryStream	    Set a binary stream parameter value.
	**	setAsciiStream	    Set an ASCII stream parameter value.
	**	setUnicodeStream    Set a Unicode stream parameter value.
	**	setCharacterStream  Set a character stream parameter value.
	**	setObject	    Set a generic object parameter value.
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
	**	paramSet		    Parameter info.
	**
	**  Private Methods:
	**
	**	prepare		    Prepare the query.
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
	**	    for advanStmt 'execute' methods which aren't supported,
	**	    and support for parameter names and flags.
	**	19-Oct-00 (gordy)
	**	    Added query_text, xactID and extracted prepare() from
	**	    the constructor so that changes in transaction state
	**	    can be tracked and the query re-prepared when necessary.
	**	 1-Nov-00 (gordy)
	**	    Support 2.0 extensions.  Added empty, getMetaData(), 
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
	*/

	class
		AdvanPrep : AdvanStmt

	{

		protected ParamSet	paramSet;

		private String    query_text = null;
	//	private AdvanQPMD prepQPMD   = null;
		private AdvanRSMD prepRSMD   = null;
		private AdvanRSMD emptyRSMD  = null;

		private int qry_concur = DrvConst.DRV_CRSR_DBMS;
		private int		xactID;
		private String	stmt_name = null;


		/*
		** Name: AdvanPrep
		**
		** Description:
		**	Class constructor.  Primary constructor for this class
		**	and derived classes which prepare the SQL text.
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
		**	    Changed parameters for 2.0 extensions.
		**	31-Oct-01 (gordy)
		**	    Timezone now passed to AdvanSQL.
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
		*/

		public
			AdvanPrep( DrvConn conn, String query, int rs_type, int rs_concur, int rs_hold ) :
			this( conn, rs_type, rs_concur, rs_hold )
		{
			title = trace.getTraceName() + "-PreparedStatement[" + inst_id + "]";
			tr_id = "Prep[" + inst_id + "]";

			if ( trace.enabled() )  trace.log( title + ": '" + query + "'" );

			try
			{
				SqlParse sql = new SqlParse( query, conn );
				query_text = sql.parseSQL( parse_escapes );
				qry_concur = sql.getConcurrency();
			}
			catch( SqlEx ex )
			{
				if ( trace.enabled() )  
					trace.log( title + ": error parsing query text" );
				if ( trace.enabled( 1 ) )  ex.trace( trace );
				throw ex;
			}

			prepare();
			return;
		} // AdvanPrep


		/*
		** Name: AdvanPrep
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
		**	    Changed parameters for 2.0 extensions.
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
			AdvanPrep( DrvConn conn, int rs_type, int rs_concur, int rs_hold ) :
			base( conn, rs_type, rs_concur, rs_hold )
		{
			paramSet = new ParamSet(conn);
			return;
		} // AdvanPrep


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

		public AdvanRslt
			executeQuery()
		{
			if ( trace.enabled() )  trace.log( title + ".executeQuery()" );

			if ( prepRSMD == null )  
			{
				if ( trace.enabled() )  
					trace.log( title + ".executeQuery(): statement is not a query" );
				clearResults();
				throw SqlEx.get( ERR_GC4017_NO_RESULT_SET );
			}

			exec( paramSet );
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
		{
			if ( trace.enabled() )  trace.log( title + ".executeUpdate()" );

			if ( prepRSMD != null )  
			{
				if ( trace.enabled() )  
					trace.log( title + ".executeUpdate(): statement is a query" );
				clearResults();
				throw SqlEx.get( ERR_GC4018_RESULT_SET_NOT_PERMITTED );
			}

			exec( paramSet );
			int count = rslt_rowcount ? rslt_val_rowcnt : 0;
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

		public virtual bool
			execute()
		{
			if ( trace.enabled() )  trace.log( title + ".execute()" );

			exec( paramSet );
			bool isRS = (resultSet != null);

			if ( trace.enabled() )  trace.log( title + ".execute(): " + isRS );
			return( isRS );
		} // execute


		/*
		** The following methods override Statement methods which are 
		** not allowed for PreparedStatement objects.
		*/

		protected internal override AdvanRslt
			executeQuery( String sql )
		{
			if ( trace.enabled() )  trace.log(title + ".executeQuery('" + sql + "')");
			throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
		} // executeQuery

		protected internal override int
			executeUpdate( String sql )
		{
			if ( trace.enabled() )  trace.log(title + ".executeUpdate('" + sql + "')");
			throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
		} // executeUpdate

		protected internal override bool 
			execute( String sql )
		{
			if ( trace.enabled() )  trace.log( title + ".execute('" + sql + "' )" );
			throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
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

		internal virtual void
			addBatch()
		{
			if ( trace.enabled() )  trace.log( title + ".addBatch()" );

			if ( prepRSMD != null )  
			{
				if ( trace.enabled() )  
					trace.log( title + ".addBatch(): statement is a query" );
				throw SqlEx.get( ERR_GC4018_RESULT_SET_NOT_PERMITTED );
			}

			if ( batch == null )  newBatch();
			lock( batch )
			{
				batch.Add( paramSet );
				paramSet = new ParamSet(conn);
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
		*/

		protected internal override int []
			executeBatch()
		{
			int[] results;

			if ( trace.enabled() )  trace.log( title + ".executeBatch()" );
			if ( batch == null )  return( new int[ 0 ] );

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
						ParamSet ps = (ParamSet)batch[0];
						batch.RemoveAt(0);
						if ( trace.enabled() )  
							trace.log( title + ".executeBatch[" + i + "] " );

						exec( ps );
						results[ i ] = rslt_rowcount ? rslt_val_rowcnt
							: SUCCESS_NO_INFO;

						if ( trace.enabled() )  
							trace.log( title + ".executeBatch[" + i + "] = " 
								+ results[ i ] );
					}
					catch (SqlEx /* ex */)
					{
						/*
						** Return only the successful query results.
						*/
						int[] temp = new int[i];
						if (i > 0)
							Array.Copy(results, 0, temp, 0, i);
						
						batch.Clear();
						throw;
					}
					catch (Exception /* ex */)
					{
						// Shouldn't happen
						/*
						** Return only the successful query results.
						*/
						int[] temp = new int[i];
						if (i > 0)
							Array.Copy(results, 0, temp, 0, i);
						results = temp;
						break;
					}
	
				batch.Clear();
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
		**	31-Oct-02 (gordy)
		**	    Adapted for generic GCF driver.  
		*/

		private void
			prepare()
		{
			xactID = conn.getXactID();
			stmt_name = conn.getStmtID( query_text, stmt_prefix );
    
			msg.LockConnection();
			try
			{
				msg.begin( MSG_QUERY );
				msg.write( MSG_QRY_PREP );
				msg.write( MSG_QP_STMT_NAME );
				msg.write( stmt_name );
				msg.write( MSG_QP_QTXT );
				msg.write( query_text );
				msg.done( true );

				prepRSMD = readResults( true );
			}
			catch( SqlEx ex )
			{
				if ( trace.enabled() )  
					trace.log( title + ": error preparing query" );
				if ( trace.enabled( 1 ) )  ex.trace( trace );
				throw ex;
			}
			finally 
			{ 
				msg.UnlockConnection(); 
			}

			return;
		} // prepare


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
		**	    Pass cursor name to result-set according to spec:
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
		*/

		private void
			exec( ParamSet param_set )
		{
			int param_cnt;

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
					throw SqlEx.get( ERR_GC4020_NO_PARAM );
				}

			/*
			** Is the current driver transaction ID different
			** than the ID obtained when the query was prepared?
			** If so, the prepared statement was lost with the
			** change in transaction state and we need to prepare
			** the query again.
			*/
			if ( xactID != conn.getXactID() )  prepare();

			msg.LockConnection();
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
					msg.write( stmt_name );
					msg.done( (param_cnt <= 0) );

					if ( param_cnt > 0 )
					{
						param_set.sendDesc(false);
						param_set.sendData(true);
					}

					readResults( timeout, true );
				}
				else
				{
					/*
					** Select statement (open a cursor).  Generate a 
					** cursor ID if not provided by the application.
					*/
					int		concurrency = getConcurrency( qry_concur );
					short	flags = 0;
					bool	needEOG = true;
					String	stmt = stmt_name;
					String	cursor = (crsr_name != null) 
						? crsr_name : conn.getUniqueID( crsr_prefix );

					/*
					** Parser removes FOR READONLY clause because it isn't
					** a part of Ingres SELECT syntax.  Tell server that
					** cursor should be READONLY (kludge older protocol
					** by appending clause to statement name).
					*/
					if ( concurrency == DrvConst.DRV_CRSR_READONLY )
						if ( conn.msg_protocol_level < MSG_PROTO_2 )
							stmt += " for readonly";
						else  if ( conn.msg_protocol_level < MSG_PROTO_3 )
							flags = (short)(flags |
								MSG_QF_READONLY);
						else
						{
							flags = (short)(flags |
								MSG_QF_READONLY | MSG_QF_FETCH_FIRST);
							needEOG = false;
						}
		    
					if ( conn.msg_protocol_level >= MSG_PROTO_3 )
						flags = (short)(flags |
							MSG_QF_AUTO_CLOSE);
		    
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
						param_set.sendDesc(false);
						param_set.sendData(true);
					}

					/*
					** Check to see if a new result-set descriptor
					** has been received.  If so, it may be different
					** than the previous descriptor and will be more
					** accurate (save only if present).
					*/
					AdvanRSMD rsmd = readResults( timeout, needEOG );
					if ( rsmd != null )  prepRSMD = rsmd;

					/*
					** The cursor name is passed to the result-set 
					** for updatable cursors or if provided by the 
					** application (2.1 API spec).
					*/
					if ( (rslt_flags & MSG_RF_READ_ONLY) == 0 )  
					{
						resultSet = new RsltUpd( conn, this, rsmd,
							rslt_val_stmt, cursor );
						if ( msg.moreMessages() )  readResults( timeout, true );
					}
					else
					{
						if ( rs_concur == DrvConst.DRV_CRSR_UPDATE )
							warnings.Add( SqlEx.get( ERR_GC4016_RS_CHANGED ));

						resultSet = new RsltCurs( conn, this, rsmd, rslt_val_stmt, 
							crsr_name, getPreFetchSize(),
							msg.moreMessages() );
					}
				}
			}
			catch( SqlEx ex )
			{
				if ( trace.enabled() )  
					trace.log( title + ".execute(): error executing query" );
				if ( trace.enabled( 1 ) )  ex.trace( trace );
				throw ex;
			}
			finally 
			{ 
				msg.UnlockConnection(); 
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
		*/

		public override void
			close()
		{
			base.close();
			paramSet.clear( true );
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

		internal AdvanRSMD
			getMetaData()
		{
			if ( trace.enabled() )  trace.log( title + ".getMetaData(): " + prepRSMD );

			if (prepRSMD == null && emptyRSMD == null)
				emptyRSMD = new AdvanRSMD((short)0, trace);

			return ((prepRSMD == null) ? emptyRSMD : prepRSMD);
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
		*/

//		public ParameterMetaData
//			getParameterMetaData()
//		{
//			if ( trace.enabled() )  trace.log( title + ".getParameterMetaData()" );
//			throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
//		} // getParameterMetaData


		/*
		** Name: getResultSetEmpty
		**
		** Description:
		**	Returns an empty result set with metdata (rsmd) filled in.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	RsltData   An empty result set with metdata (rsmd) filled in.
		**
		** History:
		**	13-Oct-05 (thoda04)
		**	    Created.
		*/
		
		internal RsltData getResultSetEmpty()
		{
			RsltData  resultset = new RsltData(conn, getMetaData(), null);
			return resultset;
		}


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
		{
				if ( trace.enabled() )  trace.log( title + ".clearParameters()" );
				paramSet.clear( false );
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
		**	sqlType		Column SQL type (ProviderType).
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
			setNull( int paramIndex, ProviderType sqlType, String typeName )
		{
			if (trace.enabled())
				trace.log(title + ".setNull( " + paramIndex + ", " + sqlType +
					   (typeName == null ? " )" : ", " + typeName + " )"));

			paramIndex = paramMap( paramIndex );
			paramSet.init( paramIndex, sqlType );
			paramSet.setNull(paramIndex);
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
			setBoolean( int paramIndex, bool value )
		{
			if ( trace.enabled() )  
				trace.log( title + ".setBoolean( " + paramIndex + ", " + value + " )" );

			paramIndex = paramMap( paramIndex );
			paramSet.init(paramIndex, ProviderType.Boolean);
			paramSet.setBoolean(paramIndex, value);
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
		{
			if ( trace.enabled() )  
				trace.log( title + ".setByte( " + paramIndex + ", " + value + " )" );

			paramIndex = paramMap( paramIndex );
			paramSet.init(paramIndex, ProviderType.TinyInt);
			paramSet.setByte(paramIndex, value);
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
		{
			if ( trace.enabled() )  
				trace.log( title + ".setShort( " + paramIndex + ", " + value + " )" );

			paramIndex = paramMap( paramIndex );
			paramSet.init(paramIndex, ProviderType.SmallInt);
			paramSet.setShort(paramIndex, value);
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
		{
			if ( trace.enabled() )  
				trace.log( title + " setInt( " + paramIndex + ", " + value + " )" );

			paramIndex = paramMap( paramIndex );
			paramSet.init(paramIndex, ProviderType.Integer);
			paramSet.setInt(paramIndex, value);
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
		{
			if ( trace.enabled() )  
				trace.log( title + ".setLong( " + paramIndex + ", " + value + " )" );

			paramIndex = paramMap( paramIndex );
			paramSet.init(paramIndex, ProviderType.BigInt);
			paramSet.setLong(paramIndex, value);
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
		{
			if ( trace.enabled() )  
				trace.log( title + ".setFloat( " + paramIndex + ", " + value + " )" );

			paramIndex = paramMap( paramIndex );
			paramSet.init(paramIndex, ProviderType.Real);
			paramSet.setFloat(paramIndex, value);
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
		{
			if ( trace.enabled() )  
				trace.log( title + ".setDouble( " + paramIndex + ", " + value + " )" );

			paramIndex = paramMap( paramIndex );
			paramSet.init(paramIndex, ProviderType.Double);
			paramSet.setDouble(paramIndex, value);
			return;
		} // setDouble

		/*
		** Name: setDecimal
		**
		** Description:
		**	Set parameter to a Decimal value.
		**
		** Input:
		**	paramIndex	Parameter index.
		**	value		Decimal value.
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
			setDecimal( int paramIndex, Decimal value )
		{
			if ( trace.enabled() )  
				trace.log( title + ".setDecimal( " + paramIndex + ", " + value + " )" );

			paramIndex = paramMap( paramIndex );
			paramSet.init(paramIndex, ProviderType.Decimal);
			paramSet.setDecimal(paramIndex, value);
			return;
		} // setDecimal

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
		{
			if ( trace.enabled() )  
				trace.log( title + ".setString( " + paramIndex + ", " + value + " )" );

			paramIndex = paramMap( paramIndex );
			paramSet.init(paramIndex, ProviderType.VarChar);
			paramSet.setString(paramIndex, value);
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
		*/

		public void
			setBytes( int paramIndex, byte[] value )
		{
			if ( trace.enabled() )  
				trace.log( title + ".setBytes( " + paramIndex + ", " + value + " )" );

			paramIndex = paramMap( paramIndex );
			paramSet.init(paramIndex, ProviderType.VarBinary);
			paramSet.setBytes(paramIndex, value);
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
			setDate( int paramIndex, DateTime value )
		{
			if ( trace.enabled() )  trace.log( title + ".setDate( " + 
										paramIndex + ", " + value + " )" );

			paramIndex = paramMap( paramIndex );
			paramSet.init(paramIndex, ProviderType.Date);
			paramSet.setDate(paramIndex, value, null );
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
			setTime( int paramIndex, DateTime value )
		{
			if ( trace.enabled() )  trace.log( title + ".setTime( " + 
										paramIndex + ", " + value + " )" );

			paramIndex = paramMap( paramIndex );
			paramSet.init(paramIndex, ProviderType.Time);
			paramSet.setTime(paramIndex, value, null );
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
			setTimestamp( int paramIndex, DateTime value )
		{

			if ( trace.enabled() )  trace.log( title + ".setTimestamp( " + 
										paramIndex + ", " + value + " )" );

			paramIndex = paramMap( paramIndex );
			paramSet.init(paramIndex, ProviderType.DateTime);
			paramSet.setTimestamp(paramIndex, value, null );
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
			setBinaryStream( int paramIndex, InputStream stream, int length )
		{
			if ( trace.enabled() )  trace.log( title + ".setBinaryStream( " + 
										paramIndex + ", " + length + " )" );

			/*
			** Check length to see if can be sent as VARBINARY.
			** Ingres won't coerce BLOB to VARBINARY, but will
			** coerce VARBINARY to BLOB, so VARBINARY is preferred.
			*/
			if ( length >= 0  &&  length <= conn.max_vbyt_len )
			{
			byte[]	bytes = new byte[ length ];

			if ( length > 0 )
				try 
				{ 
				int len = stream.read( bytes ); 

				if ( len != length )
				{
					/*
					** Byte array limits read so any difference
					** must be a truncation.
					*/
					if ( trace.enabled( 1 ) )
					trace.write( tr_id + ".setBinaryStream: read only " +
							 len + " of " + length + " bytes!" );
					setWarning( DataTruncation( paramIndex, true, 
									false, length, len ) );
				}
				}
				catch( IOException )
				{ throw SqlEx.get( ERR_GC4007_BLOB_IO ); }
			
			paramIndex = paramMap( paramIndex );
			paramSet.init(paramIndex, ProviderType.VarBinary);
			paramSet.setBytes( paramIndex, bytes );
			}
			else
			{
			paramIndex = paramMap( paramIndex );
			paramSet.init(paramIndex, ProviderType.LongVarBinary);
			paramSet.setBinaryStream( paramIndex, stream );
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
		**	    No longer need wrapper class for Reader, so use
		**	    wrapper class to convert InputStream to Reader.
		**	19-Feb-03 (gordy)
		**	    Check for alternate storage format.
		**	 1-Nov-03 (gordy)
		**	    ParamSet functionality extended.
		**	14-Sep-05 (gordy)
		**	    If stream is short enough, save as VARCHAR.
		*/

		public void
			setAsciiStream( int paramIndex, InputStream stream, int length )
		{
			if ( trace.enabled() )  trace.log( title + ".setAsciiStream( " + 
										paramIndex + ", " + length + " )" );

			/*
			** Check length to see if can be sent as VARCHAR.
			** Ingres won't coerce CLOB to VARCHAR, but will
			** coerce VARCHAR to CLOB, so VARCHAR is preferred.
			*/
			if ( length >= 0  &&  length <= (conn.ucs2_supported ? conn.max_nvch_len 
									 : conn.max_vchr_len) )
			{
			char[]  chars = new char[ length ];

			if ( length > 0 )
				try 
				{ 
				Reader	rdr = new InputStreamReader( stream, "US-ASCII" );
				int	len = rdr.read( chars ); 

				if ( len != length )
				{
					/*
					** Character array limits read so any difference
					** must be a truncation.
					*/
					if ( trace.enabled( 1 ) )
					trace.write( tr_id + ".setAsciiStream: read only " +
							 len + " of " + length + " characters!" );
					setWarning( DataTruncation( paramIndex, true, 
									false, length, len ) );
				}
				}
				catch( IOException )
				{ throw SqlEx.get( ERR_GC4007_BLOB_IO ); }

			paramIndex = paramMap( paramIndex );
			paramSet.init( paramIndex, ProviderType.VarChar );
			paramSet.setString( paramIndex, new String( chars ) );
			}
			else
			{
			paramIndex = paramMap( paramIndex );
			paramSet.init( paramIndex, ProviderType.LongVarChar );
			paramSet.setAsciiStream( paramIndex, stream );
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
		**	    2.1 spec requires UTF-8 encoding for unicode streams.
		**	19-Feb-03 (gordy)
		**	    Check for alternate storage format.
		**	 1-Nov-03 (gordy)
		**	    ParamSet functionality extended.
		*/

		public void
			setUnicodeStream( int paramIndex, InputStream stream, int length )
		{
			if ( trace.enabled() )  trace.log( title + ".setUnicodeStream( " + 
										paramIndex + ", " + length + " )" );

			paramIndex = paramMap( paramIndex );
			paramSet.init(paramIndex, ProviderType.LongVarChar);
			paramSet.setUnicodeStream(paramIndex, stream);
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
		*/

		public void
			setCharacterStream( int paramIndex, Reader reader, int length )
		{
			if ( trace.enabled() )  trace.log( title + ".setCharacterStream( " + 
										paramIndex + ", " + length + " )" );

			/*
			** Check length to see if can be sent as VARCHAR.
			** Ingres won't coerce CLOB to VARCHAR, but will
			** coerce VARCHAR to CLOB, so VARCHAR is preferred.
			*/
			if ( length >= 0  &&  length <= (conn.ucs2_supported ? conn.max_nvch_len 
									 : conn.max_vchr_len) )
			{
			char[]  chars = new char[ length ];

			if ( length > 0 )
				try 
				{ 
				int	len = reader.read( chars ); 

				if ( len != length )
				{
					/*
					** Character array limits read so any difference
					** must be a truncation.
					*/
					if ( trace.enabled( 1 ) )
					trace.write( tr_id + ".setCharacterStream: read only " +
							 len + " of " + length + " characters!" );
					setWarning( DataTruncation( paramIndex, true, 
									false, length, len ) );
				}
				}
				catch( IOException )
				{ throw SqlEx.get( ERR_GC4007_BLOB_IO ); }

			paramIndex = paramMap( paramIndex );
			paramSet.init( paramIndex, ProviderType.VarChar );
			paramSet.setString( paramIndex, new String( chars ) );
			}
			else
			{
			paramIndex = paramMap( paramIndex );
			paramSet.init( paramIndex, ProviderType.LongVarChar );
			paramSet.setCharacterStream( paramIndex, reader );
			}

			return;
		} // setCharacterStream


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
		*/

		public void
		setObject(int paramIndex, Object value)
		{
			if (trace.enabled())
				trace.log(title + ".setObject( " + paramIndex + " )");

			paramIndex = paramMap(paramIndex);
			paramSet.init(paramIndex, value);
			paramSet.setObject(paramIndex, value);
			return;
		}


		/*
		** Name: setObject
		**
		** Description:
		**	Set parameter to a generic object with the requested
		**	SQL type and scale.  A scale value of -1 indicates that the
		**	scale of the value should be used.
		**
		**	Special handling of certain objects may be requested
		**	by setting sqlType to OTHER.  If the object does not have
		**	a processing alternative, the value will processed as if
		**	setObject() with no SQL type was called (except that the
		**	scale value provided will be used for DECIMAL values).
		**
		** Input:
		**	index		Parameter index returned by paramMap().
		**	value		object.
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
		**	28-Apr-04 (wansh01)
		**	    Passed input sqlType to setObject().
		**	    For Types.OTHER, type of the object will be used for setObject().
		**	15-Sep-06 (gordy)
		**	    Support empty date strings.
		**	 3-Mar-10 (thoda04)  SIR 123368
		**	    Added support for IngresType.IngresDate parameter data type.
		*/

		public void
			setObject( int paramIndex, Object value, ProviderType sqlType, int scale, int maxsize )
		{
			if (trace.enabled()) trace.log(title + ".setObject( " + paramIndex +
							", " + sqlType + ", " + scale + " )");

			paramIndex = paramMap(paramIndex);

			if (sqlType == ProviderType.Other)
			{
				/*
				** Force alternate storage format.
				*/
				paramSet.init(paramIndex, value, true);
				sqlType = SqlData.getSqlType(value);
			}
			else
			{
				switch( sqlType )
				{
				case ProviderType.Date :
				case ProviderType.Time:
				case ProviderType.DateTime:
					/*
					** Check for an empty date string.
					*/
					if ( value is String  &&  ((String)value).Length == 0 )
					{
					/*
					** Empty dates are only supported by Ingres dates
					** which require the alternate storage format.
					*/
					paramSet.init( paramIndex, sqlType, true );
					break;
					}
					/*
					** Fall through for default behaviour.
					*/
					goto default;

				case ProviderType.IngresDate:
					goto default;

				default:
					paramSet.init( paramIndex, sqlType );
					break;
				}
			}
		 

			paramSet.setObject(paramIndex, value, sqlType, scale, maxsize);
			return;
		}


		/*
		** Data save methods which are simple covers 
		** for the primary data save methods.
		*/

		public void 
			setNull( int paramIndex, ProviderType sqlType )
		{ setNull( paramIndex, sqlType, null ); }

		public void 
		setObject( int paramIndex, Object value, ProviderType sqlType )
		{ setObject( paramIndex, value, sqlType, 0, 0 ); }




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

		protected internal virtual int
			paramMap( int index )
		{
			if ( index < 0 )  throw SqlEx.get( ERR_GC4011_INDEX_RANGE );
			return( index );
		} // paramMap


		/*
		** Name: readDesc
		**
		** Description:
		**	Read a query result data descriptor message.  Overrides the
		**	method in AdvanStmt.  Maintains a single RSMD.  Subsequent
		**	descriptor messages are loaded into the existing RSMD.
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
		**	16-Apr-01 (gordy)
		**	    Created.
		*/

		protected internal override AdvanRSMD
			readDesc()
		{
			if ( prepRSMD == null )  
				prepRSMD = AdvanRSMD.load( conn );
			else
				prepRSMD.reload( conn );

			return( prepRSMD );
		} // readDesc


		/*
		** Name: Scale
		**
		** Description:
		**	Get the scale of a decimal number.
		**
		** Input:
		**	decValue	Decimal value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	byte    	scale of the decimal number (0 to 28).
		**
		** History:
		**	16-Sep-02 (thoda04)
		**	    Created.
		*/

		private byte Scale(decimal decValue)
		{
			int[] decBits = System.Decimal.GetBits(decValue);
			//decBits[0:2] = low, middle, and high 32-bits of 96-bit integer.
			//decBits[3] = sign and scale of the decimal number.
			//	bits  0:15 = zero and are unused
			//	bits 16:23 = scale
			//	bits 24:30 = zero and are unused
			//	bit  31    = sign

			return (byte)(decBits[3]>>16);
		}

	} // class AdvanPrep

}  // namespace
