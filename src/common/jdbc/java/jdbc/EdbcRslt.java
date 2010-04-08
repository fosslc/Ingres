/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbc;

/*
** Name: EdbcRslt.java
**
** Description:
**	Abstract base class which implements the JDBC ResultSet
**	interface.
**
**  Classes:
**
**	EdbcRslt	Abstract ResultSet class.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	 8-Sep-99 (gordy)
**	    Created new base class for class which interact with
**	    the JDBC server and extracted common data and methods.
**	    Synchronize entire request/response with JDBC server.
**	13-Sep-99 (gordy)
**	    Implemented error code support.
**	29-Sep-99 (gordy)
**	    Added class to support BLOBs as streams.  Implemented
**	    BLOB data support.  Use DbConn lock()/unlock() methods
**	    for synchronization.
**	11-Nov-99 (gordy)
**	    Extracted base functionality from EdbcResult.java.
**	15-Nov-99 (gordy)
**	    Added max column length.
**	18-Nov-99 (gordy)
**	    Made string stream class nested.
**	 8-Dec-99 (gordy)
**	    Removed col_cnt since not used by this class and only
**	    used by a single sub-class.
**	 4-Feb-00 (gordy)
**	    Added closeStrm() to close input streams and which may
**	    be overriden by subclasses which track stream states.
**	 5-Sep-00 (gordy)
**	    Don't throw exception from next() when result-set is closed.
**	 4-Oct-00 (gordy)
**	    Standardized internal tracing.
**	 3-Nov-00 (gordy)
**	    Added support for JDBC 2.0 extensions.
**	23-Jan-01 (gordy)
**	    Changed parameter type to EdbcStmt for backward compatibility.
**	31-Jan-01 (gordy)
**	    Invalid dates are now handled as conversion exceptions when
**	    accessed via the getXXX() methods, rather than a protocol
**	    exception while loading the row.
**	28-Mar-01 (gordy)
**	    Added constructor.  Implemented shut() method to assume 
**	    functionality of close().  The shut() method is now called
**	    internally rather than close(), which is reserved for
**	    external access.
**	 4-Apr-01 (gordy)
**	    Generate a DataTruncation warning for 'empty' dates.
**	11-Apr-01 (gordy)
**	    Replaced timezone specific date formatters with a single
**	    date formatter which, when synchronized, may be set to
**	    whatever timezone is required.
**	13-Jun-01 (gordy)
**	    Removed UniStrm class and replaced with a method to convert
**	    UCS-2 character arrays into UTF-8 byte format.  JDBC 2.1 spec
**	    requires unicode streams to be UTF-8 encoded.
**	19-Jul-02 (gordy)
**	    Marked blob stream used after consuming contents so that sub-
**	    classes which gain control during exception conditions will
**	    be able to detect that the blob needs processing.
**	 7-Aug-02 (gordy)
**	    Return empty string for Ingres empty date.
**      10-Apr-03 (weife01) BUG 110037
**          Added parseConnDate and formatConnDate to synchronize the
**          DateFormat returned by getConnDateFormat
**	10-Jan-05 (rajus01) Startrak #:EDJDBC 95; Bug #:113722
**	    Reading Ingres intervals caused  SQLException "An invalid
**	    datatype was requested" due to side effect of the fix for
**	    bug 110037. The change in behaviour is due to change in the
**	    exception type thrown by parseConnDate(). A generic exception
**	    is being caught instead of catching a more specific exception while
**	    handling date values.  
*/

import	java.math.BigDecimal;
import	java.text.DateFormat;
import	java.util.Calendar;
import	java.util.Map;
import	java.io.InputStream;
import	java.io.ByteArrayInputStream;
import	java.io.ByteArrayOutputStream;
import	java.io.OutputStreamWriter;
import	java.io.Reader;
import	java.io.StringReader;
import	java.io.IOException;
import	java.sql.Statement;
import	java.sql.ResultSet;
import	java.sql.ResultSetMetaData;
import	java.sql.Date;
import	java.sql.Time;
import	java.sql.Timestamp;
import	java.sql.Array;
import	java.sql.Blob;
import	java.sql.Clob;
import	java.sql.Ref;
import	java.sql.Types;
import	java.sql.SQLException;
import	java.sql.DataTruncation;
import	ca.edbc.util.EdbcEx;
import	ca.edbc.io.DbConn;


/*
** Name: EdbcRslt
**
** Description:
**	EDBC JDBC Java driver class which implements the
**	base functionality of the JDBC ResultSet interface.
**	Provides access to a subset of the result-set which
**	must be loaded by a sub-class.  Support is provided 
**	for multi-row and partial-row subsets.
**
**	There is a row counter which should be incremented 
**	for each full row stored in the data subset.  There 
**	is a column counter which should be incremented for 
**	each column stored in the last row of the subset.  
**	When the entire row has been stored, the row counter 
**	is incremented and the column counter reset to 0.  A
**	partial row is indicated by a non-zero column counter
**	and applies only to the last row in the subset. Pre-
**	fetched rows are indicated by a row counter greater
**	than one.
**
**	The current row counter indicates which row of the data
**	subset is being accessed.  If the current row is beyond 
**	the limits of the row counter, then the presence of data 
**	is determined by the column counter (partial row).
**
**  Abstract Methods:
**
**	load		    Load data subset.
**
**  Interface Methods:
**
**	next		    Move to next row in the result set.
**	close		    Close the result set and free resources.
**	wasNull		    Determine if last column read was NULL.
**	getString	    Retrieve column as a string value.
**	getBoolean	    Retrieve column as a boolean value.
**	getByte		    Retrieve column as a byte value.
**	getShort	    Retrieve column as a short value.
**	getInt		    Retrieve column as a int value.
**	getLong		    Retrieve column as a long value.
**	getFloat	    Retrieve column as a float value.
**	getDouble	    Retrieve column as a double value.
**	getBigDecimal	    Retrieve column as a BigDecimal value.
**	getBytes	    Retrieve column as a byte array value.
**	getDate		    Retrieve column as a Date value.
**	getTime		    Retrieve column as a Time value.
**	getTimestamp	    Retrieve column as a Timestamp value.
**	getAsciiStream	    Retrieve column as an ASCII character stream.
**	getUnicodeStream    Retrieve column as a Unicode character stream.
**	getBinaryStream	    Retrieve column as a binary byte stream.
**	getCursorName	    Retrieve the associated cursor name.
**	getMetaData	    Create a ResultSetMetaData object.
**	getObject	    Retrieve column as a generic Java object.
**	findColumn	    Determine column index from column name.
**
**  Protected Methods:
**
**	shut		    Close the result set.
**	getAscii	    Retrieve column as an ASCII object.
**	getUnicode	    Retrieve column as a Unicode object.
**	ucs2utf8	    Convert UCS-2 to UTF-8.
**	closeStrm	    Close a column input stream.
**	columnMap	    Validates column index and returns internal index.
**
**  Protected Data:
**
**	inst_id		    Instance ID for tracing.
**	rsmd		    Result set meta-data.
**	rs_type		    Result-set type.
**	rs_concur	    Result-set concurrency.
**	rs_fetch_dir	    Fetch direction.
**	rs_fetch_size	    Pre-fetch size.
**	rs_max_len	    Max column string length.
**	rs_max_rows	    Max rows to return.
**	data_set	    Array of row/column value objects.
**	cur_col_cnt	    Number of available columns in partial row.
**	cur_row_cnt	    Number of full rows in data subset.
**	current_row	    The current row in the data subset.
**	tot_row_cnt	    Number of rows returned.
**	stream_used	    Replacement for used BLOB stream.
**	date_ex		    Generic Exception for invalid dates.
**
**  Private Data:
**
**	stmt		    Associated statement.
**	closed		    Is result set closed.
**	null_column	    Was last column retrieved NULL.
**
**  Private Methods
**
**	lvc2string	    Convert LONGVARCHAR to String.
**	lvb2array	    Convert LONGVARBINARY to byte[].
**	bin2str	    	    Convert binary to String.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	 8-Sep-99 (gordy)
**	    Created new base class for class which interact with
**	    the JDBC server and extracted common data and methods.
**	    Synchronize entire request/response with JDBC server.
**	29-Sep-99 (gordy)
**	    Implemented support for BLOB data and added support
**	    methods and fields.
**	11-Nov-99 (gordy)
**	    Extracted base functionality from EdbcResult.
**	15-Nov-99 (gordy)
**	    Added max column length.
**	 8-Dec-99 (gordy)
**	    Removed col_cnt since not used by this class and only
**	    used by a single sub-class.
**	 4-Feb-00 (gordy)
**	    Added closeStrm() to close input streams and which may
**	    be overriden by subclasses which track stream states.
**	 4-Oct-00 (gordy)
**	    Added instance counter, inst_count, and instance ID,
**	    inst_id, for standardized internal tracing.
**	 6-Nov-00 (gordy)
**	    Support JDBC 2.0 extensions.  Added stmt, rs_type, rs_concur,
**	    rs_fetch_dir, rs_fetch_size, finalize(), getStatement(), getType(), 
**	    getConcurrency(), getFetchDirection(), setFetchDirection(), 
**	    getFetchSize(), setFetchSize(), getBigDecimal(), getDate(),
**	    getTime(), getTimestamp(), getCharacterStream(), getObject(), 
**	    getArray(), getBlob(), getClob(), getRef(), cursor positioning
**	    and cursor update methods.  Renamed max_col_len to rs_max_len 
**	    and max_row_cnt to rs_max_rows.
**	23-Jan-01 (gordy)
**	    The associated statement reference must be an EdbcStmt class
**	    not a Statement interface.  We use JDBC 2.0 extensions from
**	    this object which don't exist in JDBC 1.2 and which cause
**	    execution problems when run in a JDK 1.1 environment.  Since
**	    The EdbcStmt class implements these methods, they will be
**	    available no matter what environment is used.
**	31-Jan-01 (gordy)
**	    Added date_ex to be thrown when an invalid date is detected.
**	28-Mar-01 (gordy)
**	    Added constructor for result-sets not associated with a
**	    statement.  Common constructor functionality placed in new
**	    init() method.  Implemented shut() method to assume the
**	    functionality of close().  The shut() method is now called
**	    internally rather than close(), which is reserved for
**	    external access.
**	13-Jun-01 (gordy)
**	    Removed UniStrm class and replaced with a method to convert
**	    UCS-2 character arrays into UTF-8 byte format.  JDBC 2.1 spec
**	    requires unicode streams to be UTF-8 encoded.
**      29-Aug-01 (loera01) SIR 105641
**          Made class EdbcStmt variable protected instead of private, so that
**          subclass RsltProc can have access.
*/

abstract class
EdbcRslt
    extends	EdbcObj
    implements	ResultSet
{

    protected int	    inst_id;		// Instance ID.
    protected EdbcRSMD	    rsmd = null;	// Resut set meta-data.
    protected int	    rs_type = ResultSet.TYPE_FORWARD_ONLY;
    protected int	    rs_concur = ResultSet.CONCUR_READ_ONLY;
    protected int	    rs_fetch_dir = ResultSet.FETCH_FORWARD;
    protected int	    rs_fetch_size = 0;
    protected int	    rs_max_rows = 0;	// Max rows to return.
    protected int	    rs_max_len = 0;	// Max string length to return.

    /*
    ** Multi-row data subset.
    */
    protected Object	    data_set[][] = null;
    protected int	    cur_col_cnt = 0;	// Columns in partial row.
    protected int	    cur_row_cnt = 0;	// Full rows in data_set.
    protected int	    current_row = 0;	// Active row in data_set.
    protected int	    tot_row_cnt = 0;	// Total rows returned.

    /*
    ** A BLOB stream may be used only once per row.  Once
    ** used, its data object is replaced by the stream_used
    ** object so that any attempt to re-access the BLOB can
    ** be easily detected and handled.
    */
    protected static final Object	    stream_used = new Object();

    /*
    ** Ingres date interval values don't conform to any JDBC
    ** datatype and may be detected before conversion, in
    ** which case the date_ex execption may be thrown.
    */
    protected static final Exception   date_ex = 
			   new Exception( "Invalid date format" );

    /*
    ** Local data.
    */
    private static int	    inst_count = 0;	// Number of instances.
    
    protected EdbcStmt	    stmt = null;	// Associated statement.
    private boolean	    closed = false;	// Is result set closed?
    private boolean	    null_column = false;// Was last column NULL?


/*
** Name: EdbcRslt
**
** Description:
**	Class constructor for Result-sets associated with a Statement.
**	The associated statement will be used to determine limits on 
**	the number of rows returned and length of output strings.  
**	Zero values result in the use of the default limits.
**
**	Defines a result-set supporting a known set of columns as 
**	defined by the result-set meta-data (rsmd).  The result-set 
**	can store a multiple row (size) subset of the data.  A sub-
**	class may pass 0 for the subset size, in which case it is 
**	the sub-class's responsibility to allocate the data_set array.
**
** Input:
**	stmt	    Statement generating the result-set.
**	rsmd	    Result Set Meta-Data.
**	size	    Number of rows in data subset, may be 0.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	11-Nov-99 (gordy)
**	    Created.
**	15-Nov-99 (gordy)
**	    Added max column length.
**	 4-Oct-00 (gordy)
**	    Create unique ID for standardized internal tracing.
**	 6-Nov-00 (gordy)
**	    Parameters changed to support JDBC 2.0 extensions.
**	23-Jan-01 (gordy)
**	    Changed parameter type to EdbcStmt for backward compatibility.
**	28-Mar-01 (gordy)
**	    Statement is now required.  Common functionality extracted
**	    to init() for use by constructor without statement.  Info
**	    in statement now directly accessible.
*/

protected
EdbcRslt( EdbcStmt stmt, EdbcRSMD rsmd, int size )
    throws EdbcEx
{
    super( stmt.dbc, stmt.trace );
    init();
    this.stmt = stmt;
    this.rsmd = rsmd;
    this.rs_fetch_size = stmt.rs_fetch_size;
    this.rs_max_rows = stmt.rs_max_rows;
    this.rs_max_len = stmt.rs_max_len;
    data_set = new Object[ size ][];

} // EdbcRslt


/*
** Name: EdbcRslt
**
** Description:
**	Class constructor for Result-sets which are not directly
**	associated with a statement.  Default values will be used
**	for limits on string lengths and output rows.
**
**	Defines a result-set supporting a known set of columns as 
**	defined by the result-set meta-data (rsmd).  It is the sub-
**	class's responsibility to allocate the data_set array.
**
** Input:
**	trace	    Connection tracing.
**	rsmd	    Result Set Meta-Data.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	21-Mar-01 (gordy)
**	    Created.
*/

protected
EdbcRslt( EdbcTrace trace, EdbcRSMD rsmd )
    throws EdbcEx
{
    super( null, trace );
    init();
    this.rsmd = rsmd;
}


/*
** Name: init
**
** Description:
**	Initialize common constructor items.
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
**	11-Nov-99 (gordy)
**	    Created.
**	 4-Oct-00 (gordy)
**	    Create unique ID for standardized internal tracing.
**	21-Mar-01 (gordy)
**	    Extracted from constructor to support common actions.
*/

private void
init()
{
    inst_id = inst_count++;
    title = shortTitle + "-ResultSet[" + inst_id + "]";
    tr_id = "Rslt[" + inst_id + "]";
    return;
}


/*
** Name: finalize
**
** Description:
**	Clean-up class instance.
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
**	 6-Nov-00 (gordy)
**	    Created.
*/

protected void
finalize()
    throws Throwable
{
    if ( ! closed ) try { shut(); } catch( Exception ignore ) {}
    super.finalize();
    return;
} // finalize


/*
** Abstract methods.
*/

protected abstract void	    load( int ix )  throws EdbcEx;
protected abstract void	    load()	    throws EdbcEx;


/*
** Name: next
**
** Description:
**	Position result set at next row in the result set and returns 
**	an indication if the end of the result set has been reached.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if row is valid, false if end of result set.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	 8-Sep-99 (gordy)
**	    Synchronize on DbConn.
**	29-Sep-99 (gordy)
**	    Use DbConn lock()/unlock() methods for synchronization.
**	15-Nov-99 (gordy)
**	    Close when max row count is reached.
**	13-Dec-99 (gordy)
**	    Use pre-loaded rows if available, otherwise request next
**	    subset from subclass.
**	 5-Jan-00 (gordy)
**	    Close result set at end-of-rows or an exception occurs.
**	 5-Sep-00 (gordy)
**	    Don't throw exception if result-set is closed, simply
**	    return the no-data indication.  This behavior matches
**	    ESQL and OpenAPI, as well as the SQL92 standard.
*/

public boolean
next()
    throws SQLException
{
    boolean ready = false;

    if ( trace.enabled() )  trace.log( title + ".next()" );
    warnings = null;
    null_column = false;
    
    if ( closed )  
    {
	if ( trace.enabled( 3 ) )
	    trace.write( tr_id + ".next: result-set closed" );
	if ( trace.enabled() )  trace.log( title + ".next: " + false );
	return( false );
    }

    /*
    ** Close the result set if a row limit has been set
    ** and we have previously returned the limit.
    */
    if ( rs_max_rows > 0  &&  tot_row_cnt >= rs_max_rows )
    {
	try { shut(); }  catch( EdbcEx ignore ) {}
	if ( trace.enabled( 3 ) )
	    trace.write( tr_id + ".next: max row limit reached" );
	if ( trace.enabled() )  trace.log( title + ".next: " + false );
	return( false );
    }

    /*
    ** Return the next row in the data_set if there is a full
    ** or partial row in the data set beyond the current row.
    */
    if ( (current_row + 1) < cur_row_cnt  ||
	 (current_row < cur_row_cnt  &&  cur_col_cnt > 0) )
    {
	current_row++;	    // Next row.
	tot_row_cnt++;	    // Additional rows of the data_set (see below).

	if ( trace.enabled( 3 ) )
	    trace.write(tr_id + ".next: pre-fetch row: " + (current_row + 1));
	if ( trace.enabled() )  trace.log( title + ".next: " + true );
	return( true );
    }

    /*
    ** Current subset exhausted, need to load next data subset.
    ** Sub-classes must provide a load() method to retrieve the
    ** next data subset, even if all it does is empty the data_set.
    */
    try { load(); }
    catch( EdbcEx ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".next(): error loading data" );
	if ( trace.enabled( 1 ) )  ex.trace( trace );
	current_row = cur_row_cnt = cur_col_cnt = 0;
	try { shut(); }  catch( EdbcEx ignore ) {}
	throw ex;
    }

    /*
    ** Data is available if a full row(s) or a partial row 
    ** has been received.  We automatically close the result
    ** set when there is no more data available.
    */
    if ( cur_row_cnt <= 0  &&  cur_col_cnt <= 0 )
	try { shut(); }  catch( EdbcEx ignore ) {}
    else
    {
	current_row = 0;    // First row.
	tot_row_cnt++;	    // Beginning row of the data_set (see above).
	ready = true;

	if ( trace.enabled( 3 ) )
	{
	    if ( cur_row_cnt > 0 )  
		trace.write( tr_id + ".next: rows: " + 
			     cur_row_cnt + " of " + data_set.length );
	    if ( data_set.length > 1 )  
		trace.write( tr_id + ".next: pre-fetch row: 1" );
	    if ( cur_col_cnt > 0 )  
		trace.write( tr_id + ".next: cols: " + 
			     cur_col_cnt + " of " + rsmd.count );
	}
    }

    if ( trace.enabled() )  trace.log( title + ".next: " + ready );
    return( ready );
}


/*
** Name: close
**
** Description:
**	Close the result set.
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
**	14-May-99 (gordy)
**	    Created.
**	 8-Feb-00 (gordy)
**	    Check for connection closed and complete silently.
**	28-Mar-01 (gordy)
**	    Separated into external (close()) and internal (shut())
**	    methods.
*/

public void
close()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".close()" );
    warnings = null;
    null_column = false;

    try { shut(); }
    catch( EdbcEx ex )
    {
	if ( trace.enabled() )
	    trace.log( title + ".close(): error closing result-set" );
	if ( trace.enabled( 1 ) )  ex.trace( trace );
	throw ex;
    }
    return;
} // close


/*
** Name: shut
**
** Description:
**	Internal method for closing the result-set.  Sub-classes
**	should override this method to perform their own shutdown
**	activities.  If overridden, this method should still be
**	called (super.shut()) to mark the result-set closed.
**
**	Returns an indication if the result-set had already been
**	closed.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if result-set was open, false if already closed.
**
** History:
**	28-Mar-01 (gordy)
**	    Created.
*/

boolean	// package access needed
shut()
    throws EdbcEx
{
    synchronized( this )
    {
	if ( closed )  
	{
	    if ( trace.enabled( 5 ) )  
		trace.write( tr_id + ": result-set already closed!" );
	    return( false );
	}
	closed = true;
    }

    if ( trace.enabled( 3 ) )  trace.write( tr_id + ": closing result-set" );
    current_row = cur_row_cnt = cur_col_cnt = 0;
    return( true );;
} // shut


/*
** Name: getStatement
**
** Description:
**	Retrieve the statement associated with result-set.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Statement   Associated statement.
**
** History:
**	 6-Nov-00 (gordy)
**	    Created.
*/

public Statement
getStatement()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getStatement(): " + stmt );
    return( stmt );
} // getStatement


/*
** Name: getType
**
** Description:
**	Retrieve the result-set type.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Result-set type (TYPE_FORWARD_ONLY).
**
** History:
**	 6-Nov-00 (gordy)
**	    Created.
*/

public int
getType()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getType(): " + rs_type );
    return( rs_type );
} // getType


/*
** Name: getConcurrency
**
** Description:
**	Retrieve the result-set concurrency.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Result-set concurrency (CONCUR_READ_ONLY).
**
** History:
**	 6-Nov-00 (gordy)
**	    Created.
*/

public int
getConcurrency()
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getConcurrency(): " + rs_concur );
    return( rs_concur );
} // getConcurrency


/*
** Name: getFetchDirection
**
** Description:
**	Retrieve the result-set fetch direction.
**
**	Since only TYPE_FORWARD_ONLY result-sets are supported
**	at this level, this method always returns FETCH_FORWARD.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Fetch direction (FETCH_FORWARD).
**
** History:
**	 6-Nov-00 (gordy)
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
**
** Input:
**	int	Fetch direction (must be FETCH_FORWARD).
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 6-Nov-00 (gordy)
**	    Created.
*/

public void
setFetchDirection( int direction )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setFetchDirection( " + direction + " )" );
    if ( direction != ResultSet.FETCH_FORWARD )
	throw EdbcEx.get( E_JD000A_PARAM_VALUE );
    return;
} // setFetchDirection


/*
** Name: getFetchSize
**
** Description:
**	Retrieve the result-set fetch size.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Result-set fetch size.
**
** History:
**	 6-Nov-00 (gordy)
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
**	Set the result-set fetch size.  Size must be between
**	zero and max_rows inclusive.  A zero indicates that 
**	the driver should select the best possible value.
**
** Input:
**	size	Result-set fetch size.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 6-Nov-00 (gordy)
**	    Created.
*/

public void
setFetchSize( int size )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setFetchSize( " + size + " )" );
    if ( size < 0  ||  size > rs_max_rows )
	throw EdbcEx.get( E_JD000A_PARAM_VALUE );
    rs_fetch_size = size;
    return;
} // setFetchSize


/*
** Name: getCursorName
**
** Description:
**	Returns NULL.  This method should be overridden by a sub-class
**	which supports cursor result sets.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	    Null.
**
** History:
**	 5-May-00 (gordy)
**	    Created.
*/

public String
getCursorName()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getCursorName(): " + null );
    return( null );
} // getCursorName


/*
** Name: getMetaData
**
** Description:
**	Creates a ResultSetMetaData object describing the result set.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	ResultSetMetaData   New ResultSetMetaData object.
**
** History:
**	14-May-99 (gordy)
**	    Created.
*/

public ResultSetMetaData
getMetaData()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getMetaData(): " + rsmd );
    if( closed ) throw EdbcEx.get( E_JD0013_RESULTSET_CLOSED );
    return( rsmd );
} // getMetaData


/*
** Name: getRow
**
** Description:
**	Return the current row number.  Zero returned if cursor
**	is before first (no next()) or after last (result-set
**	closed).
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Current row.
**
** History:
**	 6-Nov-00 (gordy)
**	    Created.
*/

public int
getRow()
    throws SQLException
{
    int row = closed ? 0 : tot_row_cnt;
    if ( trace.enabled() )  trace.log( title + ".getRow(): " + row );
    return( row );
} // getRow


/*
** Name: isBeforeFirst
**
** Description:
**	Is cursor before first row of the result-set.
**
**	The presence of an empty result-set cannot be determined
**	without issuing a fetch (next()), in which case the cursor
**	cannot then be before the first row.  This implementation
**	does not satisfy the JDBC specification in regards to empty
**	result-sets.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    TRUE if next() not called, FALSE otherwise.
**
** History:
**	 6-Nov-00 (gordy)
**	    Created.
*/

public boolean
isBeforeFirst()
    throws SQLException
{
    boolean val = tot_row_cnt == 0  &&  ! closed;
    if ( trace.enabled() )  trace.log( title + ".isBeforeFirst(): " + val );
    return( val );
} // isBeforeFirst


/*
** Name: isAfterLast
**
** Description:
**	Is cursor after the last row of the result-set.
**
**	Result-set is marked closed when next() returns FALSE.
**	This method should also return FALSE if result-set is
**	empty.  Therefore, the result-set must be closed and
**	there must have been at least one row returned for this
**	method to return TRUE.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    TRUE if closed, FALSE otherwise.
**
** History:
**	 6-Nov-00 (gordy)
**	    Created.
*/

public boolean
isAfterLast()
    throws SQLException
{
    boolean val = tot_row_cnt > 0  &&  closed;
    if ( trace.enabled() )  trace.log( title + ".isAfterLast(): " + val );
    return( val );
} // isAfterLast


/*
** Name: isFirst
**
** Description:
**	Is cursor on first row of the result-set.
**
**	The total row count will be one when cursor moved to
**	the first row.  Total row count will be higher or the
**	result-set will be closed if cursor moved beyond the
**	first row.
**	
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    TRUE if on first row, FALSE otherwise.
**
** History:
**	 6-Nov-00 (gordy)
**	    Created.
*/

public boolean
isFirst()
    throws SQLException
{
    boolean val = tot_row_cnt == 1  &&  ! closed;
    if ( trace.enabled() )  trace.log( title + ".isFirst(): " + val );
    return( val );
} // isFirst


/*
** Name: isLast
**
** Description:
**	Is cursor on last row of the result-set.
**
**	The last row cannot be determined without issuing 
**	a fetch() request which returns no rows, in which 
**	case the cursor is no longer on the last row.  
**	This implementation does not satisfy the JDBC 
**	specification.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    FALSE (can't determine required state).
**
** History:
**	 6-Nov-00 (gordy)
**	    Created.
*/

public boolean
isLast()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".isLast(): " + false );
    return( false );
} // isLast


/*
** Cursor positioning methods (not supported at this level).
*/

public void beforeFirst() throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".beforeFirst()" );
    throw( EdbcEx.get( E_JD0012_UNSUPPORTED ) ); 
}

public void afterLast() throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".afterLast()" );
    throw( EdbcEx.get( E_JD0012_UNSUPPORTED ) ); 
} 


public boolean first() throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".first()" );
    throw( EdbcEx.get( E_JD0012_UNSUPPORTED ) ); 
}


public boolean last() throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".last()" );
    throw( EdbcEx.get( E_JD0012_UNSUPPORTED ) ); 
}


public boolean previous() throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".previous()" );
    throw( EdbcEx.get( E_JD0012_UNSUPPORTED ) ); 
}


public boolean absolute( int row ) throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".absolute( " + row + " )" );
    throw( EdbcEx.get( E_JD0012_UNSUPPORTED ) ); 
}


public boolean relative( int rows ) throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".relative( " + rows + " )" );
    throw( EdbcEx.get( E_JD0012_UNSUPPORTED ) ); 
}


/*
** Cursor update methods (not supported at this level).
*/

public void insertRow() throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".insertRow()" );
    throw EdbcEx.get( E_JD0012_UNSUPPORTED ); 
}

public void updateRow() throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".updateRow()" );
    throw EdbcEx.get( E_JD0012_UNSUPPORTED ); 
}

public void deleteRow() throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".deleteRow()" );
    throw EdbcEx.get( E_JD0012_UNSUPPORTED ); 
}

public void refreshRow() throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".refreshRow()" );
    throw EdbcEx.get( E_JD0012_UNSUPPORTED ); 
}

public void moveToInsertRow() throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".moveToInsertRow()" );
    throw EdbcEx.get( E_JD0012_UNSUPPORTED ); 
}

public void moveToCurrentRow() throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".moveToCurrentRow()" );
    throw EdbcEx.get( E_JD0012_UNSUPPORTED ); 
}


/*
** Cursor udpate methods (not implemented at this level).
*/

public boolean rowUpdated() throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".rowUpdated(): " + false );
    return( false ); 
}

public boolean rowInserted() throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".rowInserted(): " + false );
    return( false ); 
}

public boolean rowDeleted() throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".rowDeleted(): " + false );
    return( false ); 
}

public synchronized void
cancelRowUpdates() 
    throws SQLException
{ 
    if ( trace.enabled() )  trace.log(title + ".cancelRowUpdates(): " + false);
    return; 
}

public synchronized void
updateNull( int columnIndex ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log( title + ".updateNull( " + columnIndex + " )" );
    return; 
}

public synchronized void
updateBoolean( int columnIndex, boolean value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log(title + ".updateBoolean(" + columnIndex + "," + value + ")");
    return; 
}

public synchronized void
updateByte( int columnIndex, byte value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log(title + ".updateByte(" + columnIndex + "," + value + ")");
    return; 
}

public synchronized void
updateShort( int columnIndex, short value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log(title + ".updateShort(" + columnIndex + "," + value + ")");
    return; 
}

public synchronized void
updateInt( int columnIndex, int value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log(title + ".updateInt(" + columnIndex + "," + value + ")");
    return; 
}

public synchronized void
updateLong( int columnIndex, long value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log(title + ".updateLong(" + columnIndex + "," + value + ")");
    return; 
}

public synchronized void
updateFloat( int columnIndex, float value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log(title + ".updateFloat(" + columnIndex + "," + value + ")");
    return; 
}

public synchronized void
updateDouble( int columnIndex, double value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log(title + ".updateDouble(" + columnIndex + "," + value + ")");
    return; 
}

public synchronized void
updateBigDecimal( int columnIndex, BigDecimal value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".updateBigDecimal(" + 
				       columnIndex + "," + value + ")" );
    return; 
}

public synchronized void
updateString( int columnIndex, String value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log(title + ".updateString(" + columnIndex + "," + value + ")");
    return; 
}

public synchronized void
updateBytes( int columnIndex, byte value[] ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log(title + ".updateBytes(" + columnIndex + "," + value + ")");
    return; 
}

public synchronized void
updateDate( int columnIndex, Date value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log(title + ".updateDate(" + columnIndex + "," + value + ")");
    return; 
}

public synchronized void
updateTime( int columnIndex, Time value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log(title + ".updateTime(" + columnIndex + "," + value + ")");
    return; 
}

public synchronized void
updateTimestamp( int columnIndex, Timestamp value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".updateTimestamp(" + 
				       columnIndex + "," + value + ")" );
    return; 
}

public synchronized void
updateBinaryStream( int columnIndex, InputStream in, int length ) 
    throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".updateBinaryStream(" + 
				       columnIndex + "," + length + ")" );
    return; 
}

public synchronized void
updateAsciiStream( int columnIndex, InputStream in, int length ) 
    throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".updateAsciiStream(" + 
				       columnIndex + "," + length + ")");
    return; 
}

public synchronized void
updateCharacterStream( int columnIndex, Reader in, int length ) 
    throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".updateCharacterStream(" + 
				       columnIndex + "," + length + ")" );
    return; 
}

public synchronized void 
updateObject( int columnIndex, Object value, int scale ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log( title + ".updateObject(" + 
		   columnIndex + "," + value + "," + scale + ")" );
    return; 
}


/*
** Data update methods which are simple covers for
** the primary data update methods.
*/

public void updateNull( String columnName ) throws SQLException
{ 
    updateNull( findColumn( columnName ) );
    return; 
}

public void 
updateBoolean( String columnName, boolean value ) 
    throws SQLException
{ 
    updateBoolean( findColumn( columnName ), value );
    return; 
}

public void updateByte( String columnName, byte value ) throws SQLException
{ 
    updateByte( findColumn( columnName ), value );
    return; 
}

public void updateShort( String columnName, short value ) throws SQLException
{ 
    updateShort( findColumn( columnName ), value );
    return; 
}

public void updateInt( String columnName, int value ) throws SQLException
{ 
    updateInt( findColumn( columnName ), value );
    return; 
}

public void updateLong( String columnName, long value ) throws SQLException
{ 
    updateLong( findColumn( columnName ), value );
    return; 
}

public void updateFloat( String columnName, float value ) throws SQLException
{ 
    updateFloat( findColumn( columnName ), value );
    return; 
}

public void updateDouble( String columnName, double value ) throws SQLException
{ 
    updateDouble( findColumn( columnName ), value );
    return; 
}

public void 
updateBigDecimal( String columnName, BigDecimal value ) 
    throws SQLException
{ 
    updateBigDecimal( findColumn( columnName ), value );
    return; 
}

public void updateString( String columnName, String value ) throws SQLException
{ 
    updateString( findColumn( columnName ), value );
    return; 
}

public void updateBytes( String columnName, byte value[] ) throws SQLException
{ 
    updateBytes( findColumn( columnName ), value );
    return; 
}

public void updateDate( String columnName, Date value ) throws SQLException
{ 
    updateDate( findColumn( columnName ), value );
    return; 
}

public void updateTime( String columnName, Time value ) throws SQLException
{ 
    updateTime( findColumn( columnName ), value );
    return; 
}

public void 
updateTimestamp( String columnName, Timestamp value ) 
    throws SQLException
{ 
    updateTimestamp( findColumn( columnName ), value );
    return; 
}

public void 
updateBinaryStream( String columnName, InputStream in, int length ) 
    throws SQLException
{ 
    updateBinaryStream( findColumn( columnName ), in, length );
    return; 
}

public void 
updateAsciiStream( String columnName, InputStream in, int length ) 
    throws SQLException
{ 
    updateAsciiStream( findColumn( columnName ), in, length );
    return; 
}

public void 
updateCharacterStream( String columnName, Reader in, int length ) 
    throws SQLException
{ 
    updateCharacterStream( findColumn( columnName ), in, length );
    return; 
}

public void updateObject( int columnIndex, Object value ) throws SQLException
{ 
    updateObject( columnIndex, value, -1 );
    return; 
}

public void updateObject( String columnName, Object value ) throws SQLException
{ 
    updateObject( findColumn( columnName ), value, -1 );
    return; 
}

public void 
updateObject( String columnName, Object value, int scale )  
    throws SQLException
{ 
    updateObject( findColumn( columnName ), value, scale );
    return; 
}


/*
** Name: findColumn
**
** Description:
**	Determine column index from column name.
**
** Input:
**	columnName  Name of the column.
**
** Output:
**	None.
**
** Returns:
**	int	    Column index, 0 if not found.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	04-Feb-00 (rajus01)
**	    Throw exception for invalid column name.
*/

public int
findColumn( String columnName )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".findColumn( " + columnName + " )" );
    
    for( int col = 0; col < rsmd.count; col++ )
	if ( rsmd.desc[ col ].name != null  &&
	     rsmd.desc[ col ].name.equalsIgnoreCase( columnName ) )
	{
	    col++;
	    if ( trace.enabled() )  trace.log( title + ".findColumn: " + col );
	    return( col );
	}

    if ( trace.enabled() )  trace.log(title + ".findColumn: column not found");
    throw EdbcEx.get( E_JD0014_INVALID_COLUMN_NAME );
} // findColumn


/*
** Name: wasNull
**
** Description:
**	Determine if last column retrieved was NULL.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if last column retrieved was NULL, false otherwise.
**
** History:
**	14-May-99 (gordy)
**	    Created.
*/

public boolean
wasNull()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".wasNull(): " + null_column );
    return( null_column );
} // wasNull


/*
** Name: getBoolean
**
** Description:
**	Retrieve column data as a boolean value.  If the column
**	is NULL, false is returned.
**
** Input:
**	columnIndex Column index.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if column data is boolean true, false otherwise.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented BLOB data support.
**      15-mar-01 (loera01) Bug 104332
**          Trimmed white spaces from char, varchar and long varchar before 
**          examining the string.      
**      26-apr-01 (loera01)
**          Created temporary string buffer to hold varchar and long varchars
**          to hold the results of a trim() operation.  
**	19-Jul-02 (gordy)
**	    Marked blob stream used after consuming contents so that sub-
**	    classes which gain control during exception conditions will
**	    be able to detect that the blob needs processing.
*/

public synchronized boolean
getBoolean( int columnIndex )
    throws SQLException
{
    boolean value = false;
    Object  obj;
    String tmpstr;

    if ( trace.enabled() )  
	trace.log( title + ".getBoolean( " + columnIndex + " )" );

    columnIndex = columnMap( columnIndex );
    obj = data_set[ current_row ][ columnIndex ];

    if ( obj == null )
	null_column = true;
    else
    {
	null_column = false;

	switch( rsmd.desc[ columnIndex ].sql_type )
	{
	case Types.BIT :
	    value = ((Boolean)obj).booleanValue();
	    break;

	case Types.TINYINT :
	    value = (((Byte)obj).byteValue() == 0) ? false : true;
	    break;

	case Types.SMALLINT :
	    value = (((Short)obj).shortValue() == 0) ? false : true;
	    break;

	case Types.INTEGER :
	    value = (((Integer)obj).intValue() == 0) ? false : true;
	    break;

	case Types.BIGINT :
	    value = (((Long)obj).longValue() == 0L) ? false : true;
	    break;

	case Types.REAL :
	    value = (((Float)obj).floatValue() == 0.0) ? false : true;
	    break;

	case Types.FLOAT :
	case Types.DOUBLE :
	    value = (((Double)obj).doubleValue() == 0.0) ? false : true;
	    break;

	case Types.DECIMAL :
	case Types.NUMERIC :
	    value = (((BigDecimal)obj).signum() == 0) ? false : true;
	    break;

	case Types.CHAR :
	case Types.VARCHAR :
	    tmpstr = ((String)obj).trim();
	    if (tmpstr.equals("1"))
		value = true;
            else
	        value = Boolean.valueOf( tmpstr ).booleanValue();
	    break;

	case Types.LONGVARCHAR: 
	    if ( obj == stream_used )
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( tr_id + ": attempt to re-use BLOB" );
		throw EdbcEx.get( E_JD000D_BLOB_DONE );
	    }
	    
	    tmpstr = lvc2string((Reader)obj).trim();
	    data_set[ current_row ][ columnIndex ] = stream_used;
	    if (tmpstr.equals ("1"))
		value = true;
            else
	        value = 
		    Boolean.valueOf(tmpstr).booleanValue();
	    break;

	case Types.LONGVARBINARY :
	    if ( obj != stream_used )
	    {
		closeStrm( Types.LONGVARBINARY, obj );
		data_set[ current_row ][ columnIndex ] = stream_used;
	    }
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );

	default :
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
	}
    }

    if ( trace.enabled() )  trace.log( title + ".getBoolean: " + value );
    return( value );
} // getBoolean


/*
** Name: getByte
**
** Description:
**	Retrieve column data as a byte value.  If the column
**	is NULL, zero is returned.
**
** Input:
**	columnIndex Column index.
**
** Output:
**	None.
**
** Returns:
**	byte	    Column integer value.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented BLOB data support.
**      15-Mar-01 (loera01) 104332
**          Trimmed whitespaces before calling parseByte on char, varchar 
**          and long varchar.
**	19-Jul-02 (gordy)
**	    Marked blob stream used after consuming contents so that sub-
**	    classes which gain control during exception conditions will
**	    be able to detect that the blob needs processing.
*/

public synchronized byte
getByte( int columnIndex )
    throws SQLException
{
    byte    value = 0;
    Object  obj;

    if ( trace.enabled() )  
	trace.log( title + ".getByte( " + columnIndex + " )" );

    columnIndex = columnMap( columnIndex );
    obj = data_set[ current_row ][ columnIndex ];

    if ( obj == null )
	null_column = true;
    else
    {
	null_column = false;

	switch( rsmd.desc[ columnIndex ].sql_type )
	{
	case Types.BIT : 
	    value = (byte)(((Boolean)obj).booleanValue() ? 1 : 0);
	    break;

	case Types.TINYINT :  value = ((Byte)obj).byteValue();		break;
	case Types.SMALLINT : value = ((Short)obj).byteValue();		break;
	case Types.INTEGER :  value = ((Integer)obj).byteValue();	break;
	case Types.BIGINT :   value = ((Long)obj).byteValue();		break;
	case Types.REAL :     value = ((Float)obj).byteValue();		break;
	case Types.FLOAT :
	case Types.DOUBLE :   value = ((Double)obj).byteValue();	break;
	case Types.DECIMAL :
	case Types.NUMERIC :  value = ((BigDecimal)obj).byteValue();	break;
	case Types.CHAR :
	case Types.VARCHAR :  value = Byte.parseByte( ((String)obj ).trim() );
	                      break;

	case Types.LONGVARCHAR: 
	    if ( obj == stream_used )
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( tr_id + ": attempt to re-use BLOB" );
		throw EdbcEx.get( E_JD000D_BLOB_DONE );
	    }
	    
	    value = Byte.parseByte( lvc2string( (Reader)obj ).trim() );
	    data_set[ current_row ][ columnIndex ] = stream_used;
	    break;

	case Types.LONGVARBINARY :
	    if ( obj != stream_used )
	    {
		closeStrm( Types.LONGVARBINARY, obj );
		data_set[ current_row ][ columnIndex ] = stream_used;
	    }
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );

	default :
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
	}
    }

    if ( trace.enabled() )  trace.log( title + ".getByte: " + value );
    return( value );
} // getByte


/*
** Name: getShort
**
** Description:
**	Retrieve column data as a short value.  If the column
**	is NULL, zero is returned.
**
** Input:
**	columnIndex Column index.
**
** Output:
**	None.
**
** Returns:
**	short	    Column integer value.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented BLOB data support.
**      15-Mar-01 (loera01) 104332
**          Trimmed whitespaces from char, varchar or long varchar before 
**          calling parseShort.
**	19-Jul-02 (gordy)
**	    Marked blob stream used after consuming contents so that sub-
**	    classes which gain control during exception conditions will
**	    be able to detect that the blob needs processing.
*/

public synchronized short
getShort( int columnIndex )
    throws SQLException
{
    short   value = 0;
    Object  obj;

    if ( trace.enabled() )  
	trace.log( title + ".getShort( " + columnIndex + " )" );
    
    columnIndex = columnMap( columnIndex );
    obj = data_set[ current_row ][ columnIndex ];

    if ( obj == null )
	null_column = true;
    else
    {
	null_column = false;

	switch( rsmd.desc[ columnIndex ].sql_type )
	{
	case Types.BIT : 
	    value = (short)(((Boolean)obj).booleanValue() ? 1 : 0);	
	    break;

	case Types.TINYINT :  value = ((Byte)obj).shortValue();		break;
	case Types.SMALLINT : value = ((Short)obj).shortValue();	break;
	case Types.INTEGER :  value = ((Integer)obj).shortValue();	break;
	case Types.BIGINT :   value = ((Long)obj).shortValue();		break;
	case Types.REAL :     value = ((Float)obj).shortValue();	break;
	case Types.FLOAT :
	case Types.DOUBLE :   value = ((Double)obj).shortValue();	break;
	case Types.DECIMAL :
	case Types.NUMERIC :  value = ((BigDecimal)obj).shortValue();	break;
	case Types.CHAR :
	case Types.VARCHAR :  value = Short.parseShort( ((String)obj ).trim() );
			      break;

	case Types.LONGVARCHAR: 
	    if ( obj == stream_used )
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( tr_id + ": attempt to re-use BLOB" );
		throw EdbcEx.get( E_JD000D_BLOB_DONE );
	    }
	    
	    value = Short.parseShort( lvc2string( (Reader)obj ).trim() );
	    data_set[ current_row ][ columnIndex ] = stream_used;
	    break;

	case Types.LONGVARBINARY :
	    if ( obj != stream_used )
	    {
		closeStrm( Types.LONGVARBINARY, obj );
		data_set[ current_row ][ columnIndex ] = stream_used;
	    }
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );

	default :
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
	}
    }

    if ( trace.enabled() )  trace.log( title + ".getShort: " + value );
    return( value );
} // getShort


/*
** Name: getInt
**
** Description:
**	Retrieve column data as an int value.  If the column
**	is NULL, zero is returned.
**
** Input:
**	columnIndex Column index.
**
** Output:
**	None.
**
** Returns:
**	int	    Column integer value.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented BLOB data support.
**      15-Mar-01 (loera01) 104332
**          Trimmed white spaces from char, varchar or long varchar before 
**          calling ParseInt.
**	19-Jul-02 (gordy)
**	    Marked blob stream used after consuming contents so that sub-
**	    classes which gain control during exception conditions will
**	    be able to detect that the blob needs processing.
*/

public synchronized int
getInt( int columnIndex )
    throws SQLException
{
    int	    value = 0;
    Object  obj;

    if ( trace.enabled() )  
	trace.log( title + ".getInt( " + columnIndex + " )" );

    columnIndex = columnMap( columnIndex );
    obj = data_set[ current_row ][ columnIndex ];

    if ( obj == null )
	null_column = true;
    else
    {
	null_column = false;

	switch( rsmd.desc[ columnIndex ].sql_type )
	{
	case Types.BIT : value = ((Boolean)obj).booleanValue() ? 1 : 0;	break;
	case Types.TINYINT :  value = ((Byte)obj).intValue();		break;
	case Types.SMALLINT : value = ((Short)obj).intValue();		break;
	case Types.INTEGER :  value = ((Integer)obj).intValue();	break;
	case Types.BIGINT :   value = ((Long)obj).intValue();		break;
	case Types.REAL :     value = ((Float)obj).intValue();		break;
	case Types.FLOAT :
	case Types.DOUBLE :   value = ((Double)obj).intValue();		break;
	case Types.DECIMAL :
	case Types.NUMERIC :  value = ((BigDecimal)obj).intValue();	break;
	case Types.CHAR :
	case Types.VARCHAR :  value = Integer.parseInt( ((String)obj ).trim() );
 	                      break;

	case Types.LONGVARCHAR: 
	    if ( obj == stream_used )
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( tr_id + ": attempt to re-use BLOB" );
		throw EdbcEx.get( E_JD000D_BLOB_DONE );
	    }
	    
	    value = Integer.parseInt( lvc2string( (Reader)obj ).trim() );
	    data_set[ current_row ][ columnIndex ] = stream_used;
	    break;

	case Types.LONGVARBINARY :
	    if ( obj != stream_used )
	    {
		closeStrm( Types.LONGVARBINARY, obj );
		data_set[ current_row ][ columnIndex ] = stream_used;
	    }
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );

	default :
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
	}
    }

    if ( trace.enabled() )  trace.log( title + ".getInt: " + value );
    return( value );
} // getInt


/*
** Name: getLong
**
** Description:
**	Retrieve column data as a long value.  If the column
**	is NULL, zero is returned.
**
** Input:
**	columnIndex Column index.
**
** Output:
**	None.
**
** Returns:
**	long	    Column integer value.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented BLOB data support.
**      15-Mar-01 (loera01) 104332
**          Trimmed white spaces from char, varchar or long varchar before 
**          calling ParseLong.
**	19-Jul-02 (gordy)
**	    Marked blob stream used after consuming contents so that sub-
**	    classes which gain control during exception conditions will
**	    be able to detect that the blob needs processing.
*/

public synchronized long
getLong( int columnIndex )
    throws SQLException
{
    long    value = 0;
    Object  obj;

    if ( trace.enabled() )  
	trace.log( title + ".getLong( " + columnIndex + " )" );

    columnIndex = columnMap( columnIndex );
    obj = data_set[ current_row ][ columnIndex ];

    if ( obj == null )
	null_column = true;
    else
    {
	null_column = false;

	switch( rsmd.desc[ columnIndex ].sql_type )
	{
	case Types.BIT : value = ((Boolean)obj).booleanValue() ? 1 : 0;	break;
	case Types.TINYINT :  value = ((Byte)obj).longValue();		break;
	case Types.SMALLINT : value = ((Short)obj).longValue();		break;
	case Types.INTEGER :  value = ((Integer)obj).longValue();	break;
	case Types.BIGINT :   value = ((Long)obj).longValue();		break;
	case Types.REAL :     value = ((Float)obj).longValue();		break;
	case Types.FLOAT :
	case Types.DOUBLE :   value = ((Double)obj).longValue();	break;
	case Types.DECIMAL :
	case Types.NUMERIC :  value = ((BigDecimal)obj).longValue();	break;
	case Types.CHAR :
	case Types.VARCHAR :  value = Long.parseLong( ((String)obj ).trim() );
	                      break;

	case Types.LONGVARCHAR: 
	    if ( obj == stream_used )
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( tr_id + ": attempt to re-use BLOB" );
		throw EdbcEx.get( E_JD000D_BLOB_DONE );
	    }
	    
	    value = Long.parseLong( lvc2string( (Reader)obj ).trim() );
	    data_set[ current_row ][ columnIndex ] = stream_used;
	    break;

	case Types.LONGVARBINARY :
	    if ( obj != stream_used )
	    {
		closeStrm( Types.LONGVARBINARY, obj );
		data_set[ current_row ][ columnIndex ] = stream_used;
	    }
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );

	default :
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
	}
    }

    if ( trace.enabled() )  trace.log( title + ".getLong: " + value );
    return( value );
} // getLong


/*
** Name: getFloat
**
** Description:
**	Retrieve column data as a float value.  If the column
**	is NULL, zero is returned.
**
** Input:
**	columnIndex Column index.
**
** Output:
**	None.
**
** Returns:
**	float	    Column numeric value.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented BLOB data support.
**	19-Jul-02 (gordy)
**	    Marked blob stream used after consuming contents so that sub-
**	    classes which gain control during exception conditions will
**	    be able to detect that the blob needs processing.
*/

public synchronized float
getFloat( int columnIndex )
    throws SQLException
{
    float   value = 0;
    Object  obj;

    if ( trace.enabled() )  
	trace.log( title + ".getFloat( " + columnIndex + " )" );

    columnIndex = columnMap( columnIndex );
    obj = data_set[ current_row ][ columnIndex ];

    if ( obj == null )
	null_column = true;
    else
    {
	null_column = false;

	switch( rsmd.desc[ columnIndex ].sql_type )
	{
	case Types.BIT : value = ((Boolean)obj).booleanValue() ? 1 : 0;	break;
	case Types.TINYINT :  value = ((Byte)obj).floatValue();		break;
	case Types.SMALLINT : value = ((Short)obj).floatValue();	break;
	case Types.INTEGER :  value = ((Integer)obj).floatValue();	break;
	case Types.BIGINT :   value = ((Long)obj).floatValue();		break;
	case Types.REAL :     value = ((Float)obj).floatValue();	break;
	case Types.FLOAT :
	case Types.DOUBLE :   value = ((Double)obj).floatValue();	break;
	case Types.DECIMAL :
	case Types.NUMERIC :  value = ((BigDecimal)obj).floatValue();	break;
	case Types.CHAR :
	case Types.VARCHAR :  
	    value = Float.valueOf( (String)obj ).floatValue();	
	    break;

	case Types.LONGVARCHAR: 
	    if ( obj == stream_used )
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( tr_id + ": attempt to re-use BLOB" );
		throw EdbcEx.get( E_JD000D_BLOB_DONE );
	    }
	    
	    value = Float.valueOf( lvc2string( (Reader)obj ) ).floatValue();
	    data_set[ current_row ][ columnIndex ] = stream_used;
	    break;

	case Types.LONGVARBINARY :
	    if ( obj != stream_used )
	    {
		closeStrm( Types.LONGVARBINARY, obj );
		data_set[ current_row ][ columnIndex ] = stream_used;
	    }
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );

	default :
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
	}
    }

    if ( trace.enabled() )  trace.log( title + ".getFloat: " + value );
    return( value );
} // getFloat


/*
** Name: getDouble
**
** Description:
**	Retrieve column data as a double value.  If the column
**	is NULL, zero is returned.
**
** Input:
**	columnIndex Column index.
**
** Output:
**	None.
**
** Returns:
**	double	    Column numeric value.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented BLOB data support.
**	19-Jul-02 (gordy)
**	    Marked blob stream used after consuming contents so that sub-
**	    classes which gain control during exception conditions will
**	    be able to detect that the blob needs processing.
*/

public synchronized double
getDouble( int columnIndex )
    throws SQLException
{
    double  value = 0.0;
    Object  obj;

    if ( trace.enabled() )  
	trace.log( title + ".getDouble( " + columnIndex + " )" );
    
    columnIndex = columnMap( columnIndex );
    obj = data_set[ current_row ][ columnIndex ];

    if ( obj == null )
	null_column = true;
    else
    {
	null_column = false;

	switch( rsmd.desc[ columnIndex ].sql_type )
	{
	case Types.BIT : value = ((Boolean)obj).booleanValue() ? 1 : 0;	break;
	case Types.TINYINT :  value = ((Byte)obj).doubleValue();	break;
	case Types.SMALLINT : value = ((Short)obj).doubleValue();	break;
	case Types.INTEGER :  value = ((Integer)obj).doubleValue();	break;
	case Types.BIGINT :   value = ((Long)obj).doubleValue();	break;
	case Types.REAL :     value = ((Float)obj).doubleValue();	break;
	case Types.FLOAT :
	case Types.DOUBLE :   value = ((Double)obj).doubleValue();	break;
	case Types.DECIMAL :
	case Types.NUMERIC :  value = ((BigDecimal)obj).doubleValue();	break;
	case Types.CHAR :
	case Types.VARCHAR :  
	    value = Double.valueOf( (String)obj ).doubleValue();	
	    break;

	case Types.LONGVARCHAR: 
	    if ( obj == stream_used )
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( tr_id + ": attempt to re-use BLOB" );
		throw EdbcEx.get( E_JD000D_BLOB_DONE );
	    }
	    
	    value = Double.valueOf( lvc2string( (Reader)obj ) ).doubleValue();
	    data_set[ current_row ][ columnIndex ] = stream_used;
	    break;

	case Types.LONGVARBINARY :
	    if ( obj != stream_used )
	    {
		closeStrm( Types.LONGVARBINARY, obj );
		data_set[ current_row ][ columnIndex ] = stream_used;
	    }
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );

	default :
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
	}
    }

    if ( trace.enabled() )  trace.log( title + ".getDouble: " + value );
    return( value );
} // getDouble


/*
** Name: getBigDecimal
**
** Description:
**	Retrieve column data as a BigDecimal value.  If the column
**	is NULL, null is returned.
**
** Input:
**	columnIndex Column index.
**	scale	    Number of decimal scale digits.
**
** Output:
**	None.
**
** Returns:
**	BigDecimal  Column numeric value.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented BLOB data support.
**	19-Jul-02 (gordy)
**	    Marked blob stream used after consuming contents so that sub-
**	    classes which gain control during exception conditions will
**	    be able to detect that the blob needs processing.
*/

public synchronized BigDecimal
getBigDecimal( int columnIndex, int scale )
    throws SQLException
{
    BigDecimal	value = null;
    Object	obj;

    if ( trace.enabled() )  trace.log( title + ".getBigDecimal( " + 
				       columnIndex + ", " + scale + " )" );

    columnIndex = columnMap( columnIndex );
    obj = data_set[ current_row ][ columnIndex ];

    if ( obj == null )
	null_column = true;
    else
    {
	null_column = false;

	switch( rsmd.desc[ columnIndex ].sql_type )
	{
	case Types.BIT : 
	    value = BigDecimal.valueOf( (long)( ((Boolean)obj).booleanValue() 
						? 1 : 0 ) );
	    break;

	case Types.TINYINT :  
	    value = BigDecimal.valueOf( ((Byte)obj).longValue() );
	    break;

	case Types.SMALLINT : 
	    value = BigDecimal.valueOf( ((Short)obj).longValue() );	
	    break;

	case Types.INTEGER :  
	    value = BigDecimal.valueOf( ((Integer)obj).longValue() );
	    break;

	case Types.BIGINT :   
	    value = BigDecimal.valueOf( ((Long)obj).longValue() );
	    break;
	
	case Types.REAL :     
	    value = new BigDecimal( ((Float)obj).doubleValue() );
	    break;
	
	case Types.FLOAT :
	case Types.DOUBLE :   
	    value = new BigDecimal( ((Double)obj).doubleValue() );	
	    break;

	case Types.DECIMAL :
	case Types.NUMERIC :  value = (BigDecimal)obj;	break;
	case Types.CHAR :
	case Types.VARCHAR :  value = new BigDecimal( (String)obj );	break;

	case Types.LONGVARCHAR: 
	    if ( obj == stream_used )
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( tr_id + ": attempt to re-use BLOB" );
		throw EdbcEx.get( E_JD000D_BLOB_DONE );
	    }
	    
	    value = new BigDecimal( lvc2string( (Reader)obj ) );
	    data_set[ current_row ][ columnIndex ] = stream_used;
	    break;

	case Types.LONGVARBINARY :
	    if ( obj != stream_used )
	    {
		closeStrm( Types.LONGVARBINARY, obj );
		data_set[ current_row ][ columnIndex ] = stream_used;
	    }
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );

	default :
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
	}
    
	if ( scale >= 0  &&  scale != value.scale() )  
	    value = value.setScale( scale, BigDecimal.ROUND_HALF_UP );
    }

    if ( trace.enabled() )  trace.log( title + ".getBigDecimal: " + value );
    return( value );
} // getBigDecimal


/*
** Name: getString
**
** Description:
**	Retrieve column data as a String value.  If the column
**	is NULL, null is returned.
**
** Input:
**	columnIndex Column index.
**
** Output:
**	None.
**
** Returns:
**	String	    Column string value.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented BLOB data support.
**	15-Nov-99 (gordy)
**	    Restrict length of result to max column length.
**	04-Feb-99 (rajus01)
**	    Use bin2str() to convert BLOB to String.	
**	31-Jan-01 (gordy)
**	    Invalid date formats are now handled as conversion exceptions
**	    rather than protocol errors during row loading.  Since Ingres
**	    dates may contain interval values, the exception is translated
**	    into a warning and the non-standard value is returned.
**	 4-Apr-01 (gordy)
**	    Generate a DataTruncation warning for 'empty' dates.
**	19-Jul-02 (gordy)
**	    Marked blob stream used after consuming contents so that sub-
**	    classes which gain control during exception conditions will
**	    be able to detect that the blob needs processing.
**	 7-Aug-02 (gordy)
**	    Return empty string for Ingres empty date.
*/

public synchronized String
getString( int columnIndex )
    throws SQLException
{
    String  value = null;
    Object  obj;

    if ( trace.enabled() )  
	trace.log( title + ".getString( " + columnIndex + " )" );

    columnIndex = columnMap( columnIndex );
    obj = data_set[ current_row ][ columnIndex ];
    
    if ( obj == null )
	null_column = true;
    else
    {
	null_column = false;

	switch( rsmd.desc[ columnIndex ].sql_type )
	{
	case Types.BIT :
	case Types.TINYINT :
	case Types.SMALLINT :
	case Types.INTEGER :
	case Types.BIGINT :
	case Types.REAL :
	case Types.FLOAT :
	case Types.DOUBLE :
	case Types.DECIMAL :
	case Types.NUMERIC :
	case Types.DATE :
	case Types.TIME :	value = obj.toString();	break;
	case Types.CHAR :
	case Types.VARCHAR :	value = (String)obj;	break;
	case Types.BINARY :
	case Types.VARBINARY :	value = bin2str( (byte[])obj );	break;

	case Types.TIMESTAMP :
	    try
	    {
		String str = (String)obj;

		/*
		** An Ingres date may be empty, be a date, be a timestamp
		** or be an interval.  Each of these cases is handled below.
		** An interval may be the same length as a date or timestamp,
		** but will throw a generic Exception during conversion, so even
		** a detectable interval value is handled by throwing an
		** exception.
		*/
		if ( str.length() == 0 )			// Empty
		{
		    /*
		    ** Since JDBC does not have an empty date/time concept, 
		    ** we produce a data truncation warning so the client 
		    ** can detect this special condition.
		    */
		    setWarning( new DataTruncation( columnIndex + 1, 
						    false, true, -1, 0 ) );			
		    value = str;
		}
		else if ( str.length() == D_FMT.length() )	// Date
		{
		    /*
		    ** Add default time component by converting string
		    ** to Date then to Timestamp and back to string.
		    */
		    value = (new Timestamp( 
			Date.valueOf(str).getTime() )).toString();
		}
		else  if ( str.length() == TS_FMT.length() )	// Timestamp
		{
		    /*
		    ** If connection is using local time, string has the
		    ** correct timezone and technically there is nothing
		    ** further required, especially since gateways don't
		    ** support intervals.  Just to be safe, we will always
		    ** do the conversion to check for valid format.
		    */
		    value = (new Timestamp( 
			parseConnDate(str).getTime() )).toString();
		}
		else						// Interval
		{
		    throw date_ex;  // Handled by catch block.
		}
	    }
	    catch( Exception ex )
	    {
		/*
		** This is most likely caused by an Ingres interval value.
		** Produce a warning and return the original string value.
		*/
		if ( trace.enabled( 1 ) )
		    trace.write(tr_id + ": invalid date '" + (String)obj + "'");
		setWarning( EdbcEx.getWarning( E_JD0019_INVALID_DATE ) );
		value = (String)obj;
	    }
	    break;

	case Types.LONGVARCHAR: 
	    if ( obj == stream_used )
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( tr_id + ": attempt to re-use BLOB" );
		throw EdbcEx.get( E_JD000D_BLOB_DONE );
	    }
	    
	    value = lvc2string( (Reader)obj );
	    data_set[ current_row ][ columnIndex ] = stream_used;
	    break;

	case Types.LONGVARBINARY :
	    if ( obj == stream_used )
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( tr_id + ": attempt to re-use BLOB" );
		throw EdbcEx.get( E_JD000D_BLOB_DONE );
	    }

	    value = new String( lvb2array( (InputStream)obj ) );
	    data_set[ current_row ][ columnIndex ] = stream_used;
	    break;

	default :
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
	}

	if ( rs_max_len > 0  &&  value.length() > rs_max_len )
	    value = value.substring( 0, rs_max_len );
    }

    if ( trace.enabled() )  trace.log( title + ".getString: " + value );
    return( value );
} // getString


/*
** Name: getBytes
**
** Description:
**	Retrieve column data as a byte array value.  If the column
**	is NULL, null is returned.
**
** Input:
**	columnIndex Column index.
**
** Output:
**	None.
**
** Returns:
**	byte[]	    Column binary value.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented BLOB data support.
**	15-Nov-99 (gordy)
**	    Restrict length of result to max column length.
**	19-Jul-02 (gordy)
**	    Marked blob stream used after consuming contents so that sub-
**	    classes which gain control during exception conditions will
**	    be able to detect that the blob needs processing.
*/

public synchronized byte[]
getBytes( int columnIndex )
    throws SQLException
{
    byte    value[] = null;
    Object  obj;

    if ( trace.enabled() )  
	trace.log( title + ".getBytes( " + columnIndex + " )" );
    
    columnIndex = columnMap( columnIndex );
    obj = data_set[ current_row ][ columnIndex ];
    
    if ( obj == null )
	null_column = true;
    else
    {
	null_column = false;

	switch( rsmd.desc[ columnIndex ].sql_type )
	{
	case Types.BINARY :
	case Types.VARBINARY :	value = (byte[])obj;	break;

	case Types.LONGVARBINARY :
	    if ( obj == stream_used )
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( tr_id + ": attempt to re-use BLOB" );
		throw EdbcEx.get( E_JD000D_BLOB_DONE );
	    }

	    value = lvb2array( (InputStream)obj );
	    data_set[ current_row ][ columnIndex ] = stream_used;
	    break;

	case Types.LONGVARCHAR :
	    if ( obj != stream_used )
	    {
		closeStrm( Types.LONGVARCHAR, obj );
		data_set[ current_row ][ columnIndex ] = stream_used;
	    }
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );

	default :
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
	}

	if ( rs_max_len > 0  &&  value.length > rs_max_len )
	{
	    byte ba[] = new byte[ rs_max_len ];
	    System.arraycopy( value, 0, ba, 0, rs_max_len );
	    value = ba;
	}
    }

    if ( trace.enabled() )  trace.log( title + ".getBytes: " + value );
    return( value );
} // getBytes


/*
** Name: getDate
**
** Description:
**	Retrieve column data as a Date value.  If the column
**	is NULL, null is returned.  Dates extracted from strings
**	must be JDBC date escape format: YYYY-MM-DD.
**
**	When the timezone value of a date value is not known (from
**	gateways, Ingres is always GMT), the timezone from the
**	Calendar parameter is used (local timezone if cal is NULL).
**
**	The Date value produced should have a 0 time component for
**	the requested (or local) timezone.
**
** Input:
**	columnIndex Column index.
**	cal	    Calendar for timezone, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	Date	    Column date value.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented BLOB data support.
**	 6-Nov-00 (gordy)
**	    Added Calendar parameter to permit application specification
**	    of timezone (when not known).  Timestamps are now stored as
**	    strings (require conversion).
**	31-Jan-01 (gordy)
**	    Invalid date formats are now handled as conversion exceptions
**	    rather than protocol errors during row loading.
**	 4-Apr-01 (gordy)
**	    Generate a DataTruncation warning for 'empty' dates.
**	11-Apr-01 (gordy)
**	    Replaced timezone specific date formatters with a single
**	    date formatter which, when synchronized, may be set to
**	    whatever timezone is required.
**	19-Jul-02 (gordy)
**	    Marked blob stream used after consuming contents so that sub-
**	    classes which gain control during exception conditions will
**	    be able to detect that the blob needs processing.
*/

public synchronized Date
getDate( int columnIndex, Calendar cal )
    throws SQLException
{
    Date    value = null;
    Object  obj;

    if ( trace.enabled() )  
	trace.log( title + ".getDate( " + columnIndex + ", " + cal + " )" );
    
    columnIndex = columnMap( columnIndex );
    obj = data_set[ current_row ][ columnIndex ];

    if ( obj == null )
	null_column = true;
    else
    {
	null_column = false;

	switch( rsmd.desc[ columnIndex ].sql_type )
	{
	case Types.CHAR :
	case Types.VARCHAR : 
	    try
	    {
		if ( cal == null )
		    value = Date.valueOf( (String)obj );
		else  synchronized( df_date )
		{
		    df_date.setTimeZone( cal.getTimeZone() );
		    value = new Date( df_date.parse( (String)obj ).getTime() );
		}
	    }
	    catch( Exception ex )
	    {
		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ": invalid date format" );
		throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
	    }
	    break;
	
	case Types.DATE :    
	    value = (Date)obj;
	    break;

	case Types.TIMESTAMP :	
	    try
	    {
		String str = (String)obj;
		
		/*
		** Ingres permits zero length date literals or 'empty' 
		** dates.  Since JDBC does not have any corresponding 
		** date/time concept, we use the JDBC date epoch and 
		** produce a data truncation warning so the client can 
		** detect this special condition.
		*/
		if ( str.length() == 0 )
		{
		    str = D_EPOCH;
		    setWarning( new DataTruncation( columnIndex + 1, 
						    false, true, -1, 0 ) );			
		}

		/*
		** At this point we should have either a full timestamp
		** or at least a date with no time.  Anything else is
		** probably an Ingres interval.  Ingres intervals will
		** throw a parse exception (even if matching the length
		** of a date or timestamp value) which will be caught by
		** the enclosing try block and converted to a conversion
		** exception.
		*/
		if ( str.length() == TS_FMT.length() )
		{
		    java.util.Date date;

		    /*
		    ** Need to remove the time component.  The date in 
		    ** the timezone associated with the timestamp may be 
		    ** different than the date in the requested timezone,
		    ** so first convert to GMT using the correct timezone 
		    ** for the data source.
		    */
		    if ( dbc.use_gmt_tz  ||  cal == null ) 
			date = parseConnDate( str );
		    else  synchronized( df_ts )
		    {
			df_ts.setTimeZone( cal.getTimeZone() );
			date = df_ts.parse( str );
		    }

		    /*
		    ** Now re-format (with no time component) using the 
		    ** timezone of the client to get the correct date.
		    */
		    synchronized( df_date )
		    {
			if ( cal != null )
			    df_date.setTimeZone( cal.getTimeZone() );
			else
			    df_date.setTimeZone( df_lcl.getTimeZone() );

			str = df_date.format( date );
		    }
		}

		/*
		** Value should now be in the standard DATE format.  
		*/
		if ( str.length() != D_FMT.length() )  throw date_ex;

		/*
		** Convert to a Date with a 0 time component in the
		** requested (or local) timezone.
		*/
		if ( cal == null )
		    value = Date.valueOf( str );
		else  synchronized( df_date )
		{
		    df_date.setTimeZone( cal.getTimeZone() );
		    value = new Date( df_date.parse( str ).getTime() );
		}
	    }
	    catch( Exception ex )
	    {
		if ( trace.enabled( 1 ) )
		    trace.write(tr_id + ": invalid date '" + (String)obj + "'");
		throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
	    }
	    break;

	case Types.LONGVARCHAR: 
	    if ( obj == stream_used )
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( tr_id + ": attempt to re-use BLOB" );
		throw EdbcEx.get( E_JD000D_BLOB_DONE );
	    }
	    
	    obj = lvc2string( (Reader)obj );
	    data_set[ current_row ][ columnIndex ] = stream_used;

	    try
	    {
		if ( cal == null )
		    value = Date.valueOf( (String)obj );
		else  synchronized( df_date )
		{
		    df_date.setTimeZone( cal.getTimeZone() );
		    value = new Date( df_date.parse( (String)obj ).getTime() );
		}
	    }
	    catch( Exception ex )
	    {
		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ": invalid date format" );
		throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
	    }
	    break;

	case Types.LONGVARBINARY :
	    if ( obj != stream_used )
	    {
		closeStrm( Types.LONGVARBINARY, obj );
		data_set[ current_row ][ columnIndex ] = stream_used;
	    }
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );

	default :
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
	}
    }

    if ( trace.enabled() )  trace.log( title + ".getDate: " + value );
    return( value );
} // getDate


/*
** Name: getTime
**
** Description:
**	Retrieve column data as a Time value.  If the column
**	is NULL, null is returned.  Times extracted from strings
**	must be in JDBC time escape format: HH:MM:SS.
**
**	When the timezone value of a time value is not known (from
**	gateways, Ingres is always GMT), the timezone from the
**	Calendar parameter is used (local timezone if cal is NULL).
**
**	The Date value produced should have the date component set
**	to the JDBC date EPOCH: 1970-01-01.
**
** Input:
**	columnIndex Column index.
**	cal	    Calendar for timezone, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	Time	    Column time value.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented BLOB data support.
**	 6-Nov-00 (gordy)
**	    Added Calendar parameter to permit application specification
**	    of timezone (when not known).  Timestamps are now stored as
**	    strings (require conversion).
**	31-Jan-01 (gordy)
**	    Invalid date formats are now handled as conversion exceptions
**	    rather than protocol errors during row loading.
**	 4-Apr-01 (gordy)
**	    Generate a DataTruncation warning for 'empty' dates.
**	11-Apr-01 (gordy)
**	    Replaced timezone specific date formatters with a single
**	    date formatter which, when synchronized, may be set to
**	    whatever timezone is required.
**	19-Jul-02 (gordy)
**	    Marked blob stream used after consuming contents so that sub-
**	    classes which gain control during exception conditions will
**	    be able to detect that the blob needs processing.
*/

public synchronized Time
getTime( int columnIndex, Calendar cal )
    throws SQLException
{
    Time    value = null;
    Object  obj;

    if ( trace.enabled() )  
	trace.log( title + ".getTime( " + columnIndex + " )" );
    
    columnIndex = columnMap( columnIndex );
    obj = data_set[ current_row ][ columnIndex ];

    if ( obj == null )
	null_column = true;
    else
    {
	null_column = false;

	switch( rsmd.desc[ columnIndex ].sql_type )
	{
	case Types.CHAR :
	case Types.VARCHAR :
	    try
	    {
		if ( cal == null )
		    value = Time.valueOf( (String)obj );
		else  synchronized( df_time )
		{
		    df_time.setTimeZone( cal.getTimeZone() );
		    value = new Time( df_time.parse( (String)obj ).getTime() );
		}
	    }
	    catch( Exception ex )
	    {
		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ": invalid date format" );
		throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
	    }
	    break;
	
	case Types.TIME : 
	    value = (Time)obj;
	    break;

	case Types.TIMESTAMP :	
	    try
	    {
		String		str = (String)obj;
		java.util.Date	date;

		/*
		** Ingres permits zero length date literals or 'empty' 
		** dates.  Since JDBC does not have any corresponding 
		** date/time concept, we use the JDBC date epoch and 
		** produce a data truncation warning so the client can 
		** detect this special condition.
		*/
		if ( str.length() == 0 )
		{
		    str = D_EPOCH;
		    setWarning( new DataTruncation( columnIndex + 1, 
						    false, true, -1, 0 ) );			
		}

		/*
		** At this point we should have either a full timestamp
		** or at least a date with no time.  Anything else is
		** probably an Ingres interval.  Ingres intervals will
		** throw a parse exception (even if matching the length
		** of a date or timestamp value) which will be caught by
		** the enclosing try block and converted to a conversion
		** exception.
		*/
		if ( str.length() == D_FMT.length() )
		{
		    /*
		    ** There is no time component, so create a 0 time 
		    ** value for the requested (or local) timezone.
		    */
		    if ( cal == null )
		        value = Time.valueOf( T_EPOCH );
		    else  synchronized( df_time )
		    {
			df_time.setTimeZone( cal.getTimeZone() );
			date = df_time.parse( T_EPOCH );
			value = new Time( date.getTime() );
		    }
		}
		else  if ( str.length() == TS_FMT.length() )
		{
		    /*
		    ** Need to remove the date component but retain
		    ** the correct time for the requested timezone.
		    ** First convert timestamp to GMT using the 
		    ** timezone of the data timezone.
		    */
		    if ( dbc.use_gmt_tz  ||  cal == null )
			date = parseConnDate( str );
		    else  synchronized( df_ts )
		    {
			df_ts.setTimeZone( cal.getTimeZone() );
			date = df_ts.parse( str );
		    }

		    /*
		    ** Now re-format (with no date component) using the
		    ** timezone of the client to get the correct time.
		    */
		    synchronized( df_time )
		    {
			if ( cal != null )
			    df_time.setTimeZone( cal.getTimeZone() );
			else
			    df_time.setTimeZone( df_lcl.getTimeZone() );
			
			date = df_time.parse( df_time.format( date ) );
		    }
		
		    value = new Time( date.getTime() );
		}
		else
		{
		    throw date_ex;
		}
	    }
	    catch( Exception ex )
	    {
		if ( trace.enabled( 1 ) )
		    trace.write(tr_id + ": invalid date '" + (String)obj + "'");
		throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
	    }
	    break;

	case Types.LONGVARCHAR: 
	    if ( obj == stream_used )
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( tr_id + ": attempt to re-use BLOB" );
		throw EdbcEx.get( E_JD000D_BLOB_DONE );
	    }
	    
	    obj = lvc2string( (Reader)obj );
	    data_set[ current_row ][ columnIndex ] = stream_used;

	    try
	    {
		if ( cal == null )
		    value = Time.valueOf( (String)obj );
		else  synchronized( df_time )
		{
		    df_time.setTimeZone( cal.getTimeZone() );
		    value = new Time( df_time.parse( (String)obj ).getTime() );
		}
	    }
	    catch( Exception ex )
	    {
		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ": invalid date format" );
		throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
	    }
	    break;

	case Types.LONGVARBINARY :
	    if ( obj != stream_used )
	    {
		closeStrm( Types.LONGVARBINARY, obj );
		data_set[ current_row ][ columnIndex ] = stream_used;
	    }
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );

	default :
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
	}
    }

    if ( trace.enabled() )  trace.log( title + ".getTime: " + value );
    return( value );
} // getTime


/*
** Name: getTimestamp
**
** Description:
**	Retrieve column data as a Timestamp value.  If the column
**	is NULL, null is returned.  Timestamps extracted from strings
**	must be in JDBC timestamp escape format: YYYY-MM-DD HH:MM:SS.
**
**	When the timezone value of a timestamp is not known (from
**	gateways, Ingres is always GMT), the timezone from the
**	Calendar parameter is used (local timezone if cal is NULL).
**
** Input:
**	columnIndex Column index.
**	cal	    Calendar for timezone, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	Timestamp    Column timestamp value.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented BLOB data support.
**	 6-Nov-00 (gordy)
**	    Added Calendar parameter to permit application specification
**	    of timezone (when not known).  Timestamps are now stored as
**	    strings (require conversion).
**	31-Jan-01 (gordy)
**	    Invalid date formats are now handled as conversion exceptions
**	    rather than protocol errors during row loading.
**	 4-Apr-01 (gordy)
**	    Generate a DataTruncation warning for 'empty' dates.
**	11-Apr-01 (gordy)
**	    Replaced timezone specific date formatters with a single
**	    date formatter which, when synchronized, may be set to
**	    whatever timezone is required.
**	19-Jul-02 (gordy)
**	    Marked blob stream used after consuming contents so that sub-
**	    classes which gain control during exception conditions will
**	    be able to detect that the blob needs processing.
*/

public synchronized Timestamp
getTimestamp( int columnIndex, Calendar cal )
    throws SQLException
{
    Timestamp	value = null;
    Object	obj;

    if ( trace.enabled() )  
	trace.log( title + ".getTimestamp( " + columnIndex + " )" );
    
    columnIndex = columnMap( columnIndex );
    obj = data_set[ current_row ][ columnIndex ];

    if ( obj == null )
	null_column = true;
    else
    {
	null_column = false;

	switch( rsmd.desc[ columnIndex ].sql_type )
	{
	case Types.CHAR :
	case Types.VARCHAR :
	    try
	    {
		if ( cal == null )
		    value = Timestamp.valueOf( (String)obj );
		else  synchronized( df_ts )
		{
		    df_ts.setTimeZone( cal.getTimeZone() );
		    value = new Timestamp(df_ts.parse((String)obj).getTime());
		}
	    }
	    catch( Exception ex )
	    {
		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ": invalid date format" );
		throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
	    }
	    break;

	case Types.DATE :   
	    value = new Timestamp( ((Date)obj).getTime() );
	    break;

	case Types.TIMESTAMP :	
	    try
	    {
		java.util.Date	date;
		String		str = (String)obj;
		
		/*
		** Ingres permits zero length date literals or 'empty' 
		** dates.  Since JDBC does not have any corresponding 
		** date/time concept, we use the JDBC date epoch and 
		** produce a data truncation warning so the client can 
		** detect this special condition.
		*/
		if ( str.length() == 0 )
		{
		    str = D_EPOCH;
		    setWarning( new DataTruncation( columnIndex + 1, 
						    false, true, -1, 0 ) );			
		}

		/*
		** At this point we should have either a full timestamp
		** or at least a date with no time.  Anything else is
		** probably an Ingres interval.  Ingres intervals will
		** throw a parse exception (even if matching the length
		** of a date or timestamp value) which will be caught by
		** the enclosing try block and converted to a conversion
		** exception.
		*/
		if ( str.length() == D_FMT.length() )
		{
		    /*
		    ** There is no time component, so convert to Date with a
		    ** 0 time component for the requested (or local) timezone.
		    */
		    if ( cal == null )
			date = Date.valueOf( str );
		    else  synchronized( df_date )
		    {
			df_date.setTimeZone( cal.getTimeZone() );
			date = df_date.parse( str );
		    }
		}
		else  if ( str.length() == TS_FMT.length() )
		{
		    /*
		    ** Convert to GMT using timezone of the data source.
		    */
		    if ( dbc.use_gmt_tz  ||  cal == null )
			date = parseConnDate( str );
		    else  synchronized( df_ts )
		    {
			df_ts.setTimeZone( cal.getTimeZone() );
			date = df_ts.parse( str );
		    }
		}
		else
		{
		    throw date_ex;
		}

		value = new Timestamp( date.getTime() );
	    }
	    catch( Exception ex )
	    {
		if ( trace.enabled( 1 ) )
		    trace.write(tr_id + ": invalid date '" + (String)obj + "'");
		throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
	    }
	    break;

	case Types.LONGVARCHAR: 
	    if ( obj == stream_used )
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( tr_id + ": attempt to re-use BLOB" );
		throw EdbcEx.get( E_JD000D_BLOB_DONE );
	    }
	    
	    obj = lvc2string( (Reader)obj );
	    data_set[ current_row ][ columnIndex ] = stream_used;
	    
	    try
	    {
		if ( cal == null )
		    value = Timestamp.valueOf( (String)obj );
		else  synchronized( df_ts )
		{
		    df_ts.setTimeZone( cal.getTimeZone() );
		    value = new Timestamp(df_ts.parse((String)obj).getTime());
		}
	    }
	    catch( Exception ex )
	    {
		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ": invalid date format" );
		throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
	    }
	    break;

	case Types.LONGVARBINARY :
	    if ( obj != stream_used )
	    {
		closeStrm( Types.LONGVARBINARY, obj );
		data_set[ current_row ][ columnIndex ] = stream_used;
	    }
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );

	default :
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
	}
    }

    if ( trace.enabled() )  trace.log( title + ".getTimestamp: " + value );
    return( value );
} // getTimestamp


/*
** Name: getBinaryStream
**
** Description:
**	Retrieve column data as a binary stream.  If the column
**	is NULL, null is returned.  If the column is a BLOB, the
**	associated data object is returned.  If the column can
**	be converted to an binary stream, a binary input stream
**	is returned.  Otherwise, a conversion exception is thrown.
**
** Input:
**	columnIndex Column index.
**
** Output:
**	None.
**
** Returns:
**	Object	    Binary object.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented BLOB data support.
**	11-Nov-99 (gordy)
**	    Extracted from EdbcResult (except for BLOB stream handling).
**	19-Jul-02 (gordy)
**	    Marked blob stream used after consuming contents so that sub-
**	    classes which gain control during exception conditions will
**	    be able to detect that the blob needs processing.
*/

public synchronized InputStream
getBinaryStream( int columnIndex )
    throws SQLException
{
    InputStream	value = null;
    Object	obj;

    if ( trace.enabled() )  
	trace.log( title + ".getBinaryStream( " + columnIndex + " )" );
    
    columnIndex = columnMap( columnIndex );
    obj = data_set[ current_row ][ columnIndex ];
    
    if ( obj == null )
	null_column = true;
    else
    {
	null_column = false;

	switch( rsmd.desc[ columnIndex ].sql_type )
	{
	case Types.BINARY :
	case Types.VARBINARY :	
	    value = new ByteArrayInputStream( (byte[])obj );	
	    break;

	case Types.LONGVARBINARY :
	    if ( obj == stream_used )
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( tr_id + ": attempt to re-use BLOB" );
		throw EdbcEx.get( E_JD000D_BLOB_DONE );
	    }

	    value = (InputStream)obj;
	    data_set[ current_row ][ columnIndex ] = stream_used;
	    break;

	case Types.LONGVARCHAR :
	    if ( obj != stream_used )
	    {
		closeStrm( Types.LONGVARCHAR, obj );
		data_set[ current_row ][ columnIndex ] = stream_used;
	    }
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );

	default :
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
	}
    }

    if ( trace.enabled() )  trace.log( title + ".getBinaryStream: " + value );
    return( value );
} // getBinaryStream


/*
** Name: getAscii
**
** Description:
**	Retrieve column data as an ASCII object.  If the column
**	is NULL, null is returned.  If the column is a LONGVARCHAR, 
**	a Reader instance is returned.  In all other cases, if the 
**	column can be converted to an ASCII stream, an ASCII input 
**	stream is returned.  Otherwise, a conversion exception is 
**	thrown.
**
** Input:
**	index	Internal column index.
**
** Output:
**	None.
**
** Returns:
**	Object	Column object.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented BLOB data support.
**	11-Nov-99 (gordy)
**	    Extracted from EdbcResult (except for BLOB stream handling).
**	04-Feb-99 (rajus01)
**	    Use bin2str() to convert BLOB to String.	
*/

protected Object
getAscii( int index )
    throws SQLException
{
    Object obj = data_set[ current_row ][ index ];

    if ( obj == null )
	null_column = true;
    else
    {
	null_column = false;

	switch( rsmd.desc[ index ].sql_type )
	{
	case Types.CHAR :
	case Types.VARCHAR :
	    obj = new ByteArrayInputStream( ((String)obj).getBytes() );
	    break;

	case Types.LONGVARCHAR :
	    if ( obj == stream_used )
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( tr_id + ": attempt to re-use BLOB" );
		throw EdbcEx.get( E_JD000D_BLOB_DONE );
	    }

	    data_set[ current_row ][ index ] = stream_used;
	    break;

	case Types.BINARY :
	case Types.VARBINARY :	
	    obj = new ByteArrayInputStream( (bin2str((byte[])obj)).getBytes() );
	    break;

	case Types.LONGVARBINARY :
	    if ( obj == stream_used )
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( tr_id + ": attempt to re-use BLOB" );
		throw EdbcEx.get( E_JD000D_BLOB_DONE );
	    }

	    data_set[ current_row ][ index ] = stream_used;
	    break;

	default :
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
	}
    }

    return( obj );
} // getAscii


/*
** Name: getAsciiStream
**
** Description:
**	Retrieve column data as an ASCII character stream.  If the column
**	is NULL, null is returned.  If the column can be converted to an 
**	ASCII stream, an ASCII input stream is returned.  Otherwise, a 
**	conversion exception is thrown.
**
**	At this level, character BLOBs are represented by Reader streams.
**	We could use a local class to convert from the Reader interface
**	to the required InputStream interface, but the only current sub-
**	classes which actually support underlying character BLOBs do
**	their own conversions, so the conversion is not implemented.
**
** Input:
**	columnIndex	Column index.
**
** Output:
**	None.
**
** Returns:
**	InputStream	ASCII character input stream.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented BLOB data support.
*/

public synchronized InputStream
getAsciiStream( int columnIndex )
    throws SQLException
{
    InputStream	value = null;
    Object	obj;

    if ( trace.enabled() )  
	trace.log( title + ".getAsciiStream( " + columnIndex + " )" );

    columnIndex = columnMap( columnIndex );
    obj = getAscii( columnIndex );
    
    if ( obj != null )
    {
	switch( rsmd.desc[ columnIndex ].sql_type )
	{
	case Types.LONGVARCHAR :
	    /*
	    ** For character BLOBs, getAscii() returns a
	    ** reader instance.  We just throw an exception
	    ** rather than try to convert at this level (see
	    ** comments in method description).
	    */
	    closeStrm( Types.LONGVARCHAR, obj );
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );

	default :
	    /*
	    ** getAscii() returns an InputStream for all
	    ** other column types, which we just pass on.
	    */
	    value = (InputStream)obj;
	    break;
	}
    }

    if ( trace.enabled() )  trace.log( title + ".getAsciiStream: " + value );
    return( value );
} // getAsciiStream


/*
** Name: getUnicode
**
** Description:
**	Retrieve column data as a Unicode object.  If the column
**	is NULL, null is returned.  If the column is a LONGVARCHAR, 
**	a Reader instance is returned.  In all other cases, if the 
**	column can be converted to a Unicode stream, a Unicode input 
**	stream is returned.  Otherwise, a conversion exception is 
**	thrown.
**
** Input:
**	index	Internal column index.
**
** Output:
**	None.
**
** Returns:
**	Object	Unicode object.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented BLOB data support.
**	11-Nov-99 (gordy)
**	    Extracted from EdbcResult (except for BLOB stream handling).
**	04-Feb-99 (rajus01)
**	    Use bin2str() to convert BLOB to String.
**	13-Jun-01 (gordy)
**	    Replaced UniStrm class with method which performs converts
**	    UCS2 characters to UTF-8 byte format.	
*/

public Object
getUnicode( int index )
    throws SQLException
{
    Object obj = data_set[ current_row ][ index ];
    
    if ( obj == null )
	null_column = true;
    else
    {
	null_column = false;

	switch( rsmd.desc[ index ].sql_type )
	{
	case Types.CHAR :
	case Types.VARCHAR :
	    {
		char chr[] = ((String)obj).toCharArray();
		obj = new ByteArrayInputStream( ucs2utf8(chr,0,chr.length) );
	    }
	    break;

	case Types.LONGVARCHAR :
	    if ( obj == stream_used )
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( tr_id + ": attempt to re-use BLOB" );
		throw EdbcEx.get( E_JD000D_BLOB_DONE );
	    }

	    data_set[ current_row ][ index ] = stream_used;
	    break;

	case Types.BINARY :
	case Types.VARBINARY :
	    {
		char chr[] = bin2str( (byte[])obj ).toCharArray();
		obj = new ByteArrayInputStream( ucs2utf8(chr,0,chr.length) );
	    }
	    break;

	case Types.LONGVARBINARY :
	    if ( obj == stream_used )
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( tr_id + ": attempt to re-use BLOB" );
		throw EdbcEx.get( E_JD000D_BLOB_DONE );
	    }

	    data_set[ current_row ][ index ] = stream_used;
	    break;

	default :
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
	}
    }

    return( obj );
} // getUnicode

/*
** Name: getUnicodeStream
**
** Description:
**	Retrieve column data as a Unicode stream.  If the column
**	is NULL, null is returned.  If the column can be converted 
**	to a Unicode stream, a Unicode input stream is returned.  
**	Otherwise, a conversion exception is thrown.
**
**	At this level, character BLOBs are represented by Reader streams.
**	We could use a local class to convert from the Reader interface
**	to the required InputStream interface, but the only current sub-
**	classes which actually support underlying character BLOBs do
**	their own conversions, so the conversion is not implemented here.
**
** Input:
**	columnIndex Column index.
**
** Output:
**	None.
**
** Returns:
**	InputStream Unicode character input stream.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented BLOB data support.
*/

public synchronized InputStream
getUnicodeStream( int columnIndex )
    throws SQLException
{
    InputStream	value = null;
    Object	obj;

    if ( trace.enabled() )  
	trace.log( title + ".getUnicodeStream( " + columnIndex + " )" );
    
    columnIndex = columnMap( columnIndex );
    obj = getUnicode( columnIndex );

    if ( obj != null )
    {
	switch( rsmd.desc[ columnIndex ].sql_type )
	{
	case Types.LONGVARCHAR :
	    /*
	    ** For character BLOBs, getUnicode() returns a
	    ** reader instance.  We just throw an exception
	    ** rather than try to convert at this level (see
	    ** comments in method description).
	    */
	    closeStrm( Types.LONGVARCHAR, obj );
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );

	default :
	    /*
	    ** getUnicode() returns an InputStream for all
	    ** other column types, which we just pass on.
	    */
	    value = (InputStream)obj;
	    break;
	}
    }

    if ( trace.enabled() )  trace.log( title + ".getUnicodeStream: " + value );
    return( value );
} // getUnicodeStream


/*
** Name: getCharacter
**
** Description:
**	Retrieve column data as a Character object.  If the column
**	is NULL, null is returned.  If the column is a LONGVARBINARY,
**	an input stream is returned.  In all other cases, if the 
**	column can be converted to a string, a Reader instance is 
**	returned.  Otherwise, a conversion exception is thrown.
**
** Input:
**	index	Internal column index.
**
** Output:
**	None.
**
** Returns:
**	Object	Column object.
**
** History:
**	 6-Nov-00 (gordy)
**	    Created.
*/

protected Object
getCharacter( int index )
    throws SQLException
{
    Object obj = data_set[ current_row ][ index ];

    if ( obj == null )
	null_column = true;
    else
    {
	null_column = false;

	switch( rsmd.desc[ index ].sql_type )
	{
	case Types.CHAR :
	case Types.VARCHAR :
	    obj = new StringReader( (String)obj );
	    break;

	case Types.LONGVARCHAR :
	    if ( obj == stream_used )
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( tr_id + ": attempt to re-use BLOB" );
		throw EdbcEx.get( E_JD000D_BLOB_DONE );
	    }

	    data_set[ current_row ][ index ] = stream_used;
	    break;

	case Types.BINARY :
	case Types.VARBINARY :	
	    obj = new StringReader( bin2str((byte[])obj) );
	    break;

	case Types.LONGVARBINARY :
	    if ( obj == stream_used )
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( tr_id + ": attempt to re-use BLOB" );
		throw EdbcEx.get( E_JD000D_BLOB_DONE );
	    }

	    data_set[ current_row ][ index ] = stream_used;
	    break;

	default :
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
	}
    }

    return( obj );
} // getCharacter


/*
** Name: getCharacterStream
**
** Description:
**	Retrieve column data as a character stream.  If the column
**	is NULL, null is returned.  If the column can be converted to a 
**	character stream, a character Reader is returned.  Otherwise, a 
**	conversion exception is thrown.
**
** Input:
**	columnIndex Column index.
**
** Output:
**	None.
**
** Returns:
**	Reader	    Character stream.
**
** History:
**	 6-Nov-00 (gordy)
**	    Created.
*/

public synchronized Reader
getCharacterStream( int columnIndex )
    throws SQLException
{
    Reader	value = null;
    Object	obj;

    if ( trace.enabled() )  
	trace.log( title + ".getCharacterStream( " + columnIndex + " )" );

    columnIndex = columnMap( columnIndex );
    obj = getCharacter( columnIndex );
    
    if ( obj != null )
    {
	switch( rsmd.desc[ columnIndex ].sql_type )
	{
	case Types.LONGVARBINARY :
	    /*
	    ** For binary BLOBs, getCharacter() returns 
	    ** an InputStream.  We just throw an exception
	    ** rather than try to convert at this level 
	    ** (see comments in method description).
	    */
	    closeStrm( Types.LONGVARBINARY, obj );
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );

	default :
	    /*
	    ** getCharacter() returns a Reader for all
	    ** other column types, which we just pass on.
	    */
	    value = (Reader)obj;
	    break;
	}
    }

    if ( trace.enabled() )  trace.log(title + ".getCharacterStream: " + value);
    return( value );
} // getCharacterStream


/*
** Name: getArray
**
** Description:
**	Retrieve column data as an array.  If the column 
**	is NULL, null is returned.
**
** Input:
**	columnIndex Column index.
**
** Output:
**	None.
**
** Returns:
**	Array	    Column array value.
**
** History:
**	 6-Nov-00 (gordy)
**	    Created.
*/

public synchronized Array
getArray( int columnIndex )
    throws SQLException
{
    if (trace.enabled())  trace.log(title + ".getArray( " + columnIndex + " )");
    throw EdbcEx.get( E_JD0012_UNSUPPORTED );
} // getArray


/*
** Name: getBlob
**
** Description:
**	Retrieve column data as a Blob.  If the column 
**	is NULL, null is returned.
**
** Input:
**	columnIndex Column index.
**
** Output:
**	None.
**
** Returns:
**	Blob	    Column Blob value.
**
** History:
**	 6-Nov-00 (gordy)
**	    Created.
*/

public synchronized Blob
getBlob( int columnIndex )
    throws SQLException
{
    if (trace.enabled())  trace.log(title + ".getBlob( " + columnIndex + " )");
    throw EdbcEx.get( E_JD0012_UNSUPPORTED );
} // getBlob


/*
** Name: getClob
**
** Description:
**	Retrieve column data as a Clob.  If the column 
**	is NULL, null is returned.
**
** Input:
**	columnIndex Column index.
**
** Output:
**	None.
**
** Returns:
**	Clob	    Column Clob value.
**
** History:
**	 6-Nov-00 (gordy)
**	    Created.
*/

public synchronized Clob
getClob( int columnIndex )
    throws SQLException
{
    if (trace.enabled())  trace.log(title + ".getClob( " + columnIndex + " )");
    throw EdbcEx.get( E_JD0012_UNSUPPORTED );
} // getClob


/*
** Name: getRef
**
** Description:
**	Retrieve column data as a Ref.  If the column 
**	is NULL, null is returned.
**
** Input:
**	columnIndex Column index.
**
** Output:
**	None.
**
** Returns:
**	Ref	    Column Ref value.
**
** History:
**	 6-Nov-00 (gordy)
**	    Created.
*/

public synchronized Ref
getRef( int columnIndex )
    throws SQLException
{
    if (trace.enabled())  trace.log(title + ".getRef( " + columnIndex + " )");
    throw EdbcEx.get( E_JD0012_UNSUPPORTED );
} // getRef


/*
** Name: getObject
**
** Description:
**	Retrieve column data as a generic Java object.  If the column
**	is NULL, null is returned.
**
** Input:
**	columnIndex Column index.
**
** Output:
**	None.
**
** Returns:
**	Object	    Java object.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented BLOB data support.
**	15-Nov-99 (gordy)
**	    Restrict length of result to max column length.
**	 6-Nov-00 (gordy)
**	    Timestamps now saved as strings (conversion required).
**	 4-Apr-01 (gordy)
**	    Generate a DataTruncation warning for 'empty' dates.
**	19-Jul-02 (gordy)
**	    Marked blob stream used after consuming contents so that sub-
**	    classes which gain control during exception conditions will
**	    be able to detect that the blob needs processing.
*/

public synchronized Object
getObject( int columnIndex )
    throws SQLException
{
    Object obj;

    if ( trace.enabled() )  
	trace.log( title + ".getObject( " + columnIndex + " )" );
    
    columnIndex = columnMap( columnIndex );
    obj = data_set[ current_row ][ columnIndex ];
    
    if ( obj == null )
	null_column = true;
    else
    {
	null_column = false;

	/*
	** Most of the data values are already stored in
	** objects of the correct type.  Convert those
	** few whose Edbc object types are different than
	** the JDBC specification.
	*/
	switch( rsmd.desc[ columnIndex ].sql_type )
	{
	case Types.TINYINT :  
	    obj = new Integer( ((Byte)obj).intValue() );  
	    break;

	case Types.SMALLINT : 
	    obj = new Integer( ((Short)obj).intValue() ); 
	    break;

	case Types.CHAR :
	case Types.VARCHAR :
	    if ( rs_max_len > 0  &&  ((String)obj).length() > rs_max_len )
		obj = ((String)obj).substring( 0, rs_max_len );
	    break;

	case Types.BINARY :
	case Types.VARBINARY :
	    if ( rs_max_len > 0  &&  ((byte[])obj).length > rs_max_len )
	    {
		byte ba[] = new byte[ rs_max_len ];
		System.arraycopy( (byte[])obj, 0, ba, 0, rs_max_len );
		obj = ba;
	    }
	    break;

	case Types.TIMESTAMP :
	    java.util.Date	date;
	    String		str = (String)obj;

	    /*
	    ** Ingres permits zero length date literals or 'empty' 
	    ** dates.  Since JDBC does not have any corresponding 
	    ** date/time concept, we use the JDBC date epoch and 
	    ** produce a data truncation warning so the client can 
	    ** detect this special condition.
	    */
	    if ( str.length() == 0 )  
	    {
		str = D_EPOCH;
		setWarning( new DataTruncation( columnIndex + 1, 
						    false, true, -1, 0 ) );			
	    }

	    /*
	    ** At this point we should have either a full timestamp
	    ** or at least a date with no time.  Anything else is
	    ** probably an Ingres interval.  Ingres intervals will
	    ** throw a parse exception (even if matching the length
	    ** of a date or timestamp value) which will be caught by
	    ** the enclosing try block and converted to a conversion
	    ** exception.
	    */
            if ( str.length() == D_FMT.length() )
	        date = Date.valueOf( str );
	    else  
	        date = parseConnDate( str );

	    obj = new Timestamp( date.getTime() );
	    
	    break;

	case Types.LONGVARCHAR: 
	    if ( obj == stream_used )
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( tr_id + ": attempt to re-use BLOB" );
		throw EdbcEx.get( E_JD000D_BLOB_DONE );
	    }
	    
	    obj = lvc2string( (Reader)obj );
	    data_set[ current_row ][ columnIndex ] = stream_used;
	    break;

	case Types.LONGVARBINARY :
	    if ( obj == stream_used )
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( tr_id + ": attempt to re-use BLOB" );
		throw EdbcEx.get( E_JD000D_BLOB_DONE );
	    }

	    obj = lvb2array( (InputStream)obj );
	    data_set[ current_row ][ columnIndex ] = stream_used;
	    break;
	}
    }

    return( obj );
} // getObject


/*
** Data access methods which are simple covers for
** the primary data access method.
*/

public boolean getBoolean( String columnName ) throws SQLException
{ return( getBoolean( findColumn( columnName ) ) ); } 

public byte getByte( String columnName ) throws SQLException
{ return( getByte( findColumn( columnName ) ) ); } 

public short getShort( String columnName ) throws SQLException
{ return( getShort( findColumn( columnName ) ) ); }

public int getInt( String columnName ) throws SQLException
{ return( getInt( findColumn( columnName ) ) ); }

public long getLong( String columnName ) throws SQLException
{ return( getLong( findColumn( columnName ) ) ); }

public float getFloat( String columnName ) throws SQLException
{ return( getFloat( findColumn( columnName ) ) ); }

public double getDouble( String columnName ) throws SQLException
{ return( getDouble( findColumn( columnName ) ) ); }

public BigDecimal 
getBigDecimal( String columnName, int scale ) 
    throws SQLException
{ return( getBigDecimal( findColumn( columnName ), scale ) ); }

public BigDecimal getBigDecimal( int columnIndex ) throws SQLException
{ return( getBigDecimal( columnIndex, -1 ) ); }

public BigDecimal getBigDecimal( String columnName ) throws SQLException
{ return( getBigDecimal( columnName, -1 ) ); }

public String getString( String columnName ) throws SQLException
{ return( getString( findColumn( columnName ) ) ); }

public byte[] getBytes( String columnName ) throws SQLException
{ return( getBytes( findColumn( columnName ) ) ); }

public Date getDate( int columnIndex ) throws SQLException
{ return( getDate( columnIndex, null ) ); }

public Date getDate( String columnName ) throws SQLException
{ return( getDate( findColumn( columnName ), null ) ); }

public Date getDate( String columnName, Calendar cal ) throws SQLException
{ return( getDate( findColumn( columnName ), cal ) ); }

public Time getTime( int columnIndex ) throws SQLException
{ return( getTime( columnIndex, null ) ); }

public Time getTime( String columnName ) throws SQLException
{ return( getTime( findColumn( columnName ), null ) ); }

public Time getTime( String columnName, Calendar cal ) throws SQLException
{ return( getTime( findColumn( columnName ), cal ) ); }

public Timestamp getTimestamp( int columnIndex ) throws SQLException
{ return( getTimestamp( columnIndex, null ) ); }

public Timestamp getTimestamp( String columnName ) throws SQLException
{ return( getTimestamp( findColumn( columnName ), null ) ); }

public Timestamp 
getTimestamp( String columnName, Calendar cal ) 
    throws SQLException
{ return( getTimestamp( findColumn( columnName ), cal ) ); }

public InputStream getBinaryStream( String columnName ) throws SQLException
{ return( getBinaryStream( findColumn( columnName ) ) ); }

public InputStream getAsciiStream( String columnName ) throws SQLException
{ return( getAsciiStream( findColumn( columnName ) ) ); }

public InputStream getUnicodeStream( String columnName ) throws SQLException
{ return( getUnicodeStream( findColumn( columnName ) ) ); }

public Reader getCharacterStream( String columnName ) throws SQLException
{ return( getCharacterStream( findColumn( columnName ) ) ); }

public Array getArray( String columnName ) throws SQLException
{ return( getArray( findColumn( columnName ) ) ); }

public Blob getBlob( String columnName ) throws SQLException
{ return( getBlob( findColumn( columnName ) ) ); }

public Clob getClob( String columnName ) throws SQLException
{ return( getClob( findColumn( columnName ) ) ); }

public Ref getRef( String columnName ) throws SQLException
{ return( getRef( findColumn( columnName ) ) ); } 

public Object getObject( String columnName ) throws SQLException
{ return( getObject( findColumn( columnName ) ) ); }

public Object getObject( int columnIndex, Map map ) throws SQLException
{ return( getObject( columnIndex ) ); }

public Object getObject( String columnName, Map map ) throws SQLException
{ return( getObject( findColumn( columnName ) ) ); }


/*
** Name: closeStrm
**
** Description:
**	Close a stream associated with a column.  Supports both
**	Reader streams (LONGVARCHAR columns) and InputStream
**	streams (LONGVARBINARY columns).
**
**	This method should be overridden by subclasses which track
**	the state of input streams.
**
** Input:
**	type	    Stream type (LONGVARCHAR or LONGVARBINARY).
**	strm	    The input strm object (Reader or InputStream).
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 4-Feb-00 (gordy)
**	    Created.
*/

protected void
closeStrm( int type, Object strm )
    throws EdbcEx
{
    switch( type )
    {
    case Types.LONGVARCHAR :
	try { ((Reader)strm).close(); }
	catch( Exception ignore ) {}
	break;

    case Types.LONGVARBINARY :
	try { ((InputStream)strm).close(); }
	catch( Exception ignore ) {}
	break;
    }

    return;
} // closeStrm


/*
** Name: columnMap
**
** Description:
**	Determines if a given column index is a part of the result
**	set and maps the external index to the internal index.
**
** Input:
**	index	    External column index [1,rsmd.count].
**
** Output:
**	None.
**
** Returns:
**	int	    Internal column index [0,rsmd.count - 1].
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	 4-Apr-01 (gordy)
**	    Clear warnings as common functionality.
**	18-May-01 (gordy)
**	    Throw more meaningful exception when closed.
*/

protected int
columnMap( int index )
    throws EdbcEx
{
    warnings = null;

    if ( closed ) throw EdbcEx.get( E_JD0013_RESULTSET_CLOSED );

    if ( index < 1  ||  index > rsmd.count  ||
	 (cur_row_cnt == 0  &&  cur_col_cnt == 0) )
	throw EdbcEx.get( E_JD0007_INDEX_RANGE );

    /*
    ** Internally, column indexes start at 0.
    */
    index--;

    /*
    ** If request is for a column of a partial row which
    ** is beyond what is currently available, request that
    ** data be loaded upto the current requested column.
    */
    if ( 
	 current_row == cur_row_cnt  &&  
	 cur_col_cnt > 0  &&  
	 index >= cur_col_cnt 
       )  
	load( index );

    return( index );
} // columnMap


/*
** Name: lvc2string
**
** Description:
**	Read a character input stream (long varchar) and convert
**	to a string object.
**
** Input:
**	in	Input character stream.
**
** Output:
**	None.
**
** Returns:
**	String	The BLOB as a string.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
**	15-Nov-99 (gordy)
**	    Restrict length of result to max column length.
*/

private String
lvc2string( Reader in )
    throws EdbcEx
{
    StringBuffer    sb = new StringBuffer();

    try
    {
	char	cb[] = new char[ 8192 ];
	int	len;

	while( (len = in.read( cb, 0, cb.length )) >= 0 )
	    if ( rs_max_len <= 0 )
		sb.append( cb, 0, len );
	    else
	    {
		sb.append( cb, 0, Math.min(len, rs_max_len - sb.length()) );
		if ( sb.length() >= rs_max_len )  break;
	    }
    }
    catch( IOException ex )
    {
	throw EdbcEx.get( E_JD000B_BLOB_IO );
    }
    finally
    {
	closeStrm( Types.LONGVARCHAR, in );
    }

    return( sb.toString() );
}

/*
** Name: lvb2array
**
** Description:
**	Read a binary input stream (long binary) and convert
**	to a byte array.
**
** Input:
**	in	Input binary stream.
**
** Output:
**	None.
**
** Returns:
**	byte[]	The BLOB as a byte array.
**
** History:
**	29-Sep-99 (gordy)
**	    Created.
**	15-Nov-99 (gordy)
**	    Restrict length of result to max column length.
*/

private byte []
lvb2array( InputStream in )
    throws EdbcEx
{
    byte ba[] = new byte[ 0 ];

    try
    {
	byte	buff[] = new byte[ 8192 ];
	int	len;

	while( (len = in.read( buff, 0, buff.length )) >= 0 )
	{
	    byte tmp[];

	    if ( rs_max_len > 0 )
		len = Math.min( len, rs_max_len - ba.length );
	    tmp = new byte[ ba.length + len ];
	    System.arraycopy( ba, 0, tmp, 0, ba.length );
	    System.arraycopy( buff, 0, tmp, ba.length, len );
	    ba = tmp;
	    if ( rs_max_len > 0  &&  ba.length >= rs_max_len )  break;
	}
    }
    catch( IOException ex )
    {
	throw EdbcEx.get( E_JD000B_BLOB_IO );
    }
    finally
    {
	closeStrm( Types.LONGVARBINARY, in );
    }

    return( ba );
}

/*
** Name: bin2str
**
** Description:
**	Convert a byte array and to a hex string.
**
** Input:
**	bytes	Byte array.
**
** Output:
**	None.
**
** Returns:
**	String	Hex string representation.
**
** History:
**	31-Jan-99 (rajus01)
**	    Created.
*/

private String
bin2str( byte bytes[] )
{
    StringBuffer	sb = new StringBuffer();
    String		str;

    for( int i = 0; i < bytes.length; i++ )
    {
	str = Integer.toHexString( (int)bytes[ i ] & 0xff );
	if ( str.length() == 1 ) sb.append( '0' );
	sb.append( str );
    }

    return( new String( sb ) );	
} // bin2str


/*
** Name ucs2utf8
**
** Description:
**	Convert a character array to a UTF-8 byte array.
**
**	This is hopelessly inefficient, but the JDBC UTF-8
**	interfaces have been deprecated so hopefully this
**	won't get used much.
**
** Input:
**	ucs2	Character array.
**	offset	Start of data.
**	length	Length of data.
**
** Output:
**	None.
**
** Returns:
**	byte[]	UTF-8 representation.
**
** History:
**	13-Jun-01 (gordy)
**	    Created.
*/

protected byte[]
ucs2utf8( char ucs2[], int offset, int length )
    throws SQLException
{
    byte utf8[] = null;

    try
    {
	ByteArrayOutputStream outUTF8 = new ByteArrayOutputStream(length * 3);
	OutputStreamWriter outUCS2 = new OutputStreamWriter(outUTF8, "UTF-8");

	outUCS2.write( ucs2, offset, length );
	outUCS2.flush();
	utf8 = outUTF8.toByteArray();
	outUCS2.close();
	outUTF8.close();
    }
    catch( Exception ex )
    {
	throw EdbcEx.get( E_JD0006_CONVERSION_ERR ); 
    }

    return( utf8 );
} // ucs2utf8

} // class EdbcRslt

