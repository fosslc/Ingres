/*
** Copyright (c) 1999, 2010 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: RsltFtch.java
**
** Description:
**	Defines class which implements the JDBC ResultSet interface
**	representing a data-set fetched from a server.
**
**  Classes
**
**	RsltFtch
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
**	    Extracted base functionality to EdbcRslt.java.
**	12-Nov-99 (gordy)
**	    Use configured date formatter.
**	15-Nov-99 (gordy)
**	    Added max row count and max column length.
**	18-Nov-99 (gordy)
**	    Made BLOB stream classes nested.
**	23-Nov-99 (gordy)
**	    Handle Ingres 'empty' dates.
**	 8-Dec-99 (gordy)
**	    Since col_cnt is only relevent to BLOBS and this result
**	    set class, made it a private member of this class.
**	13-Dec-99 (gordy)
**	    Added support for pre-fetching of rows.
**	 5-Jan-00 (gordy)
**	    Close result set at end-of-rows or an exception occurs.
**	25-Jan-00 (gordy)
**	    Added tracing.
**	04-Feb-00 (rajus01)
**	    Added getMetaData().
**	19-May-00 (gordy)
**	    Track closed status.  Use class methods for locking to
**	    isolate lock strategy and permit override by sub-classes.
**	    Close cursor when end-of-data is reached.
**	 4-Oct-00 (gordy)
**	    Standardized internal tracing.
**	27-Oct-00 (gordy)
**	    Removed locking (responsibility of sub-classes) and renamed
**	    to RsltFtch.
**	 3-Nov-00 (gordy)
**	    Added support for JDBC 2.0 extensions.
**	23-Jan-01 (gordy)
**	    Changed parameter type to EdbcStmt for backward compatibility.
**	31-Jan-01 (gordy)
**	    Don't throw protocol exceptions for invalid date formats.
**	    EdbcRslt now handles these as conversion errors.
**	28-Mar-01 (gordy)
**	    Combined functionality of close() into shut().  Renamed
**	    shut() to closeCursor() and created new shut().
**	 4-Apr-01 (gordy)
**	    Empty Date DataTruncation warnings moved to EdbcRslt.
**	18-Apr-01 (gordy)
**	    Use readBytes() method which reads length from input message.
**	10-May-01 (gordy)
**	    Support UCS2 transport format for character datatypes.
**	19-Jun-02 (gordy)
**	    BUG 108286 - Protocol error closing result-set with blob column
**	    which was not accessed with getXXX().  Changed the processing 
**	    order in shut() to perform the local class close operations 
**	    before calling the super class shut().
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	20-Mar-03 (gordy)
**	    Updated to JDBC 3.0.
**	 4-Aug-03 (gordy)
**	    Removed row cacheing from JdbcRslt and implemented as a
**	    dynamically sized list.
**	20-Aug-03 (gordy)
**	    Check for automatic cursor closure.
**	22-Sep-03 (gordy)
**	    SQL data objects now encapsulate data values.
**	 6-Oct-03 (gordy)
**	    Added pre-loading ability to result-sets.
**	 1-Nov-03 (gordy)
**	    Implemented updatable result-sets.
**	15-Mar-04 (gordy)
**	    Support BIGINT columns.
**	10-May-05 (gordy)
**	    Don't throw exception when server has already closed cursor.
**	16-Jun-06 (gordy)
**	    ANSI Date/Time data type support.
**	15-Nov-06 (gordy)
**	    LOB Locator support.
**	26-Feb-07 (gordy)
**	    Support buffered LOBs.
**	 6-Apr-07 (gordy)
**	    Support scrollable cursors.
**	 4-May-07 (gordy)
**	    Set class access for reflection.
**	20-Jul-07 (gordy)
**	    Row class now encapsulates a ResultSet row.
**	24-Dec-08 (gordy)
**	    Use date/time formatters associated with connection.
**      05-Jan-09 (rajus01) SIR 121238
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to
**            support JDBC 4.0 SQLException hierarchy.
**	26-Oct-09 (gordy)
**	    Added support for BOOLEAN data type.
**	 9-Aug-10 (gordy)
**	    Track highest row received.  This provides some information
**	    about the size of the result set prior to determining the
**	    complete size.
*/

import	java.util.Stack;
import	java.util.Vector;
import	java.sql.Statement;
import	java.sql.Types;
import	java.sql.SQLException;
import	java.sql.DataTruncation;
import	com.ingres.gcf.dam.MsgConst;
import	com.ingres.gcf.util.DbmsConst;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.util.SqlData;
import	com.ingres.gcf.util.SqlNull;
import	com.ingres.gcf.util.SqlTinyInt;
import	com.ingres.gcf.util.SqlSmallInt;
import	com.ingres.gcf.util.SqlInt;
import	com.ingres.gcf.util.SqlBigInt;
import	com.ingres.gcf.util.SqlReal;
import	com.ingres.gcf.util.SqlDouble;
import	com.ingres.gcf.util.SqlDecimal;
import	com.ingres.gcf.util.SqlBool;
import	com.ingres.gcf.util.IngresDate;
import	com.ingres.gcf.util.SqlDate;
import	com.ingres.gcf.util.SqlTime;
import	com.ingres.gcf.util.SqlTimestamp;
import	com.ingres.gcf.util.SqlByte;
import	com.ingres.gcf.util.SqlVarByte;
import	com.ingres.gcf.util.SqlLongByte;
import	com.ingres.gcf.util.SqlLongByteCache;
import	com.ingres.gcf.util.SqlChar;
import	com.ingres.gcf.util.SqlNChar;
import	com.ingres.gcf.util.SqlVarChar;
import	com.ingres.gcf.util.SqlNVarChar;
import	com.ingres.gcf.util.SqlLongChar;
import	com.ingres.gcf.util.SqlLongCharCache;
import	com.ingres.gcf.util.SqlLongNChar;
import	com.ingres.gcf.util.SqlLongNCharCache;
import	com.ingres.gcf.util.SqlLoc;
import	com.ingres.gcf.util.SqlStream;


/*
** Name: RsltFtch
**
** Description:
**	JDBC driver class implementing the JDBC ResultSet interface
**	for result data fetched from a server.  While not abstract,
**	this class should be sub-classed to provide data source 
**	specific support and connection locking.
**
**	This class supports both multi-row fetching (pre-fetch) and
**	partial row fetches (for LOB support), though the two are
**	not expected to occur at the same time.  LOB columns result 
**	in partial row processing and a LOB value is not available 
**	once processing passes beyond the LOB column.  Pre-fetch
**	is automatically disabled for result-sets containing LOBs.
**	Result-sets containing LOBs should not be pre-loaded.
**
**	Column processing is interrupted when a LOB column is read.
**	The column counter is set to indicate that the column has
**	been loaded even though the actual contents of the LOB
**	stream remain to be read.  The active stream is saved and
**	the reference, activeStream, may be used by sub-classes to
**	check for active LOB columns.  This class implements the
**	SQL stream listener interface to receive notification of
**	the stream closure.  Column processing continues via a
**	call to the resume() method.  Active streams are forcefully
**	closed (flush()) when unread columns following the LOB are 
**	referenced and for other requests which require server 
**	communications.
**
**	This class does not provide connection locking support which
**	is left as a responsibility of sub-classes.  Methods which 
**	require locking support are fetch, resume(), and closeCursor().  
**	The following conditions may be assumed when these methods 
**	are called: the connection is dormant when fetch() and 
**	closeCursor() are called, active when resume() is called.  
**	The connection will be active if a stream is active when 
**	fetch() or resume() return.  Otherwise, the connection will 
**	be dormant upon exit from all three methods.
**
**	Rows are read into a row-set.  The current cursor state 
**	and position should be initialized prior to loading rows
**	to indicate the targeted result-set position.  The cursor
**	position is updated as rows are loaded.  Non-row conditions,
**	such as BEFORE, AFTER, or DELETED, must be determined after
**	loading has completed.  When there are rows in the row-set,
**	the cursor is either positioned on the last row in the set
**	or is AFTER the last row.  When the row-set is empty, the
**	cursor is either BEFORE or AFTER the result-set.
**	
**	A logical offset identifies the current row position within
**	the row-set.  A value of 0 indicates the BEFORE position
**	and is only valid when one of following conditions exists:
**
**	  * Row-set is not empty and first row in row-set is also
**	    the first row of the result-set.
**	  * Row-set is empty and cursor is BEFORE the result-set.
**	  * Row-set is empty, cursor is AFTER the result-set,
**	    and result-set is empty.
**
**	A logical offset of rowSet.size()+1 indicate the AFTER
**	position and is only valid when the cursor is AFTER the
**	result-set.  Otherwise, the logical offset ranges between
**	1 and rowSet.size() and indicates a current row in the
**	row-set (0 to rowSet.size()-1).  After loading rows, the
**	logical row will generally be first row in the rowset.
**	The logical offset and row state must be determined after 
**	loading rows.
**
**	The highest row fetched is tracked in maxRow.  Prior to
**	fetching any rows, maxRow is set to 0.  If an empty result
**	set is found, both maxRow and rowCount are set to 0.
**
**  Interface Methods:
**
**	streamClosed		SQL Stream was closed.
**
**  Overridden Methods:
**
**	getStatement		Return associated statement.
**	load			Load next row.
**	shut			Close result-set.
**	flush			Finish processing current row.
**	setProcResult		Save procedure return value.
**	readData		Read data from server.
**
**  Protected Data:
**
**	maxRow			Highest row found.
**	activeStream		Active LOB stream.
**
**  Protected Methods:
**
**	fetchSize		Number of rows to fetch.
**	preLoad			Pre-load the row cache.
**	fetch			Request and load rows/columns from DBMS.
**	resume			Resume message processing after LOB.
**	closeCursor		Close cursor.
**
**  Private Data:
**
**	rowSet			Current row-set.
**	freeCache		Unused rows.
**	logicalRow		Current row in row-set.
**	cursorStatus		Physical cursor status.
**	cursorRow		Physical cursor row.
**	stmt			Associated statement.
**	stmtID			DBMS statement ID.
**	maxFetch		Max fetch limit.
**	preFetch		Suggested fetch size.
**
**  Private Methods:
**
**	rowSetAtBOD		Row-set is at start of result-set.
**	rowSetAtEOD		Row-set is at end of result-set.
**	clearRowset		Free row-set resources.
**	rowInRowset		Load row from current row-set.
**	createRow		Allocate row and column data objects.
**	readColumns		Read column data from server.
**	calcTarget		Calculate target position.
**	checkResults		Check FETCH results.
**	updateRow		Set the current cursor position.
**	updateStatus		Set the current cursor state.
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
**	    Extracted base functionality to EdbcRslt.
**	15-Nov-99 (gordy)
**	    Added row count and max row count.
**	 8-Dec-99 (gordy)
**	    Since col_cnt is only relevent to BLOBS and this result
**	    set class, made it a private member of this class.
**	13-Dec-99 (gordy)
**	    Added data_set, cur_row_cnt, current_row for pre-fetching.
**	 4-Feb-00 (gordy)
**	    Override the closeStrm() method to process columns whose
**	    streams are closed by the superclass such as when a stream
**	    is retrieved through the getString() method.
**	 4-May-00 (gordy)
**	    Extracted multi-row handling to base class.
**	19-May-00 (gordy)
**	    Added closed field for tracking status, and lock(), unlock()
**	    methods to permit sub-classes to control locking mode.
**	27-Oct-00 (gordy)
**	    Removed locking and cursor related fields/methods.  Renamed
**	    to RsltFtch.
**	28-Mar-01 (gordy)
**	    Renamed shut() to closeCursor() and closed to cursorClosed()
**	    Created new shut() method with functionality of close().
**	31-Oct-02 (gordy)
**	    Super-class now indicates BLOB closure with blobClosed() method.
**	    JDBC statement info no longer maintained by super-class.  Added
**	    local stmt_id to hide DrvObj field used for DONE message results.
**	 4-Aug-03 (gordy)
**	    Pre-fetch and partial row support moved fully into this class.
**	    Added data_set and associated counters.  Renamed load() methods
**	    to fetch() and implemented load() as an extraction of super-
**	    class next() functionality.  Extracted readColumns() from
**	    readData().
**	22-Sep-03 (gordy)
**	    Changed blob_active flag into activeStream to track the current
**	    active SQL Stream data value.  Implemented stream listener
**	    interface (streamClosed()).  Separated row/column allocation
**	    and the reading of column data values (allocateRowBuffer()).
**	 6-Oct-03 (gordy)
**	    Added setProcResult() to generically handle procedure results
**	    (RsltProc class dropped).  Added preLoad() to handle row data
**	    returned with initial query response.  Moved row cache init
**	    to common method, initRowCache(), for preLoad() and load().
**	 1-Nov-03 (gordy)
**	    Made flush() methods protected so that they may be called by
**	    sub-classes.
**	 6-Apr-07 (gordy)
**	    Added fetch() method to support positioned fetches.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, permit access.
**	20-Jul-07 (gordy)
**	    New row-set representation based on Row class.
**	 9-Aug-10 (gordy)
**	    Added maxRow to track highest record received.
*/

public class
RsltFtch
    extends JdbcRslt
    implements MsgConst, DbmsConst, SqlStream.StreamListener
{

    protected int	maxRow = 0;			// Max row read
    protected SqlStream	activeStream = null;		// Active LOB stream

    /*
    ** Cache capacities will be set explicitly for
    ** the expected row-set size, so initial values
    ** should avoid over-optimizations.
    */
    private Vector	rowSet = new Vector( 0, 1 );	// Current row-set
    private Stack	freeCache = new Stack();	// Unused rows

    /*
    ** Initial cursor state represents a logical
    ** and physical cursor state of BEFORE.
    */
    private int		logicalRow = 0;			// Current logical row
    private int		cursorStatus = BEFORE;		// Cursor status
    private int		cursorRow = 0;			// Cursor row

    private JdbcStmt	stmt = null;			// Associated statement
    private long	stmtID = 0;			// Server statement ID
    private int		maxFetch = 0;			// Max fetch limit
    private int		preFetch = 0;			// Fetch size
    
    
/*
** Name: RsltFtch
**
** Description:
**	Class constructor.
**
** Input:
**	conn		Associated connection.
**	stmt		Associated statement.
**	rsmd		ResultSet meta-data.
**	stmtID		Statement ID.
**	maxFetch	Max fetch limit.
**	preFetch	Suggested fetch size.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	15-Nov-99 (gordy)
**	    Added max row count and max column length.
**	13-Dec-99 (gordy)
**	    Added fetch limit and multi-row data set.
**	 4-Oct-00 (gordy)
**	    Create unique ID for standardized internal tracing.
**	 3-Nov-00 (gordy)
**	    Parameters changed for JDBC 2.0 extensions.
**	23-Jan-01 (gordy)
**	    Changed parameter type to EdbcStmt for backward compatibility.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.  Statement info maintained here.
**	 4-Aug-03 (gordy)
**	    Pre-fetch handling moved entirely into this class.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, restrict constructor access.
**	20-Jul-07 (gordy)
**	    Pre-fetching now restricted via max fetch limit.
*/

protected
RsltFtch
( 
    DrvConn	conn,
    JdbcStmt	stmt, 
    JdbcRSMD	rsmd, 
    long	stmtID,
    int		maxFetch,
    int		preFetch
)
    throws SQLException
{
    super( conn, rsmd );
    
    this.stmt = stmt;
    this.stmtID = stmtID;
    this.maxFetch = maxFetch;
    this.preFetch = preFetch;

    rs_fetch_dir = stmt.rs_fetch_dir;
    rs_fetch_size = stmt.rs_fetch_size;
    rs_max_len = stmt.rs_max_len;
    rs_max_rows = stmt.rs_max_rows;
    tr_id = "Ftch[" + inst_id + "]";
    
    /*
    ** Row pre-fetching must be disabled if there is a LOB column.
    */
    for( int col = 0; col < rsmd.count; col++ )
	if ( rsmd.desc[ col ].sql_type == Types.LONGVARCHAR  ||
    	     rsmd.desc[ col ].sql_type == Types.LONGVARBINARY )
	{
	    maxFetch = 1;
	    break;
	}
    
    return;
} // RsltFtch


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
**	31-Oct-02 (gordy)
**	    Override super-class implementation to expose statement info.
*/

public Statement
getStatement()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getStatement(): " + stmt );
    return( stmt );
} // getStatement


/*
** Name: fetchSize
**
** Description:
**	Determine the number of rows to fetch based on the
**	suggested size and restricted by any max limits.
**	At a minimum, a fetch size of 1 will be returned.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Fetch size.
**
** History:
**	20-Jul-07 (gordy)
**	    Created.
*/

protected int
fetchSize()
{
    int rows = 1;	// Default fetch size

    /*
    ** Use suggested fetch size
    */
    if ( rs_fetch_size > 0 )
	rows = rs_fetch_size;
    else  if ( preFetch > 0 )
	rows = preFetch;

    /*
    ** Restrict fetch size to max limits.
    */
    if ( maxFetch > 0  &&  rows > maxFetch )
	rows = maxFetch;
    if ( rs_max_rows > 0  &&  rows > rs_max_rows )
	rows = rs_max_rows;

    return( rows );
} // fetchSize


/*
** Name: preLoad
**
** Description:
**	Load rows returned with initial query response.
**
**	This method is expected to be called by the constructor of a
**	sub-class.  It is provided as a separate method rather than
**	as a constructor option of this class so that the sub-class
**	is given a chance to do its own initialization prior to pre-
**	loading.
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
**	 6-Oct-03 (gordy)
**	    Created.
**	20-Jul-07 (gordy)
**	    Set-up row-set.
*/

protected void
preLoad()
    throws SQLException
{
    if ( msg.moreMessages() )	// Make sure there is something to read
    {
	/*
	** Set-up to load first row in result-set.
	*/
	calcTarget( ROW, 1 );

	/*
	** The number of rows fetched during pre-loading
	** is the internal suggested pre-fetch size.
	*/
	int rows = (preFetch > 0) ? preFetch : 1;
	rowSet.ensureCapacity( rows );
	freeCache.ensureCapacity( rows );

	readResults();
	checkResults( true );
    }
    
    return;
} // preLoad


/*
** Name: load
**
** Description:
**	Position the result set to the next row and load the column
**	values.  
**
**	Retrieves next row from row cache or fetches rows from server.
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
**	 8-Sep-99 (gordy)
**	    Synchronize on DbConn.
**	29-Sep-99 (gordy)
**	    Use DbConn lock()/unlock() methods for synchronization.
**	 5-May-00 (gordy)
**	    Extracted multi-row handling to EdbcRslt and renamed load().
**	19-May-00 (gordy)
**	    Use class lock(), unlock() methods to isolate lock strategy.
**	    Check for closed state.  Close the cursor when end-of-data 
**	    is reached.
**	27-Oct-00 (gordy)
**	    Removed locking (sub-class responsibility).
**	28-Mar-01 (gordy)
**	    Renamed close() to shut() and shut() to closeCursor().
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 4-Aug-03 (gordy)
**	    Row caching now implemented entirely in this class.  Actual
**	    server row fetching moved to fetch().
**	22-Sep-03 (gordy)
**	    Replaced BLOB indicator with stream reference.
**	 6-Oct-03 (gordy)
**	    Moved row cache initialization to common method.
**	 6-Apr-07 (gordy)
**	    Super class now calls flush().
**	20-Jul-07 (gordy)
**	    Functionality moved to overloaded load() which supports scrolling.
*/

protected void
load()
    throws SQLException
{
    if ( trace.enabled( 3 ) )  trace.write( tr_id + ".load()" );

    load( ROW, 1, fetchSize() );
    return;
} // load


/*
** Name: load
**
** Description:
**	Position the result set to a specific row.  If row exists
**	in current row-set, the cached value is returned.  Other-
**	wise, a request is made to the server to fetch the row.
**
** Input:
**	reference	Reference position.
**	offset		Row offset from reference.
**	rows		Number of rows to fetch.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 6-Apr-07 (gordy)
**	    Created.
**	20-Jul-07 (gordy)
**	    New row-set representation.
*/

protected void
load( int reference, int offset, int rows )
    throws SQLException
{
    /*
    ** Load row from current row-set if present.
    */
    if ( rowInRowset( reference, offset ) )  return;

    /*
    ** A reference relative to the current row must be adjusted
    ** to compensate for the difference between the logical and
    ** physical cursor positions.
    */
    if ( reference == ROW )
    {
	/*
	** The cursor is either positioned BEFORE the row-set,
	** on the last ROW of the row-set, or AFTER the row-set.
	** Because a physical cursor position of BEFORE is only
	** associated with an empty row-set, the logical position
	** of the cursor for the BEFORE and ROW cases is the same
	** as the row-set size.  In the AFTER case, it is one
	** greater than the row-set size.
	*/
	int position = (cursorStatus == AFTER  ||  cursorStatus == CLOSED)
			? rowSet.size() + 1 : rowSet.size();

	/*
	** The logical row position should never be greater than
	** the physical cursor, so the offset is decreased by
	** the difference.
	*/
	offset -= position - logicalRow;
    }

    /*
    ** Load new row-set.
    **
    ** Row-set is initialized and target position calculated.
    ** Note: order of operations is designed to require least
    ** overhead in row caches.
    */
    if ( rows > 0 )  freeCache.ensureCapacity( rows );
    clearRowset();
    if ( rows > 0 )  rowSet.ensureCapacity( rows );
    calcTarget( reference, offset );

    fetch( reference, offset, rows );
    checkResults( rows > 0 );

    /*
    ** If no rows were loaded, cursor must be at BOD or EOD.
    ** Otherwise, the first row in the row-set is returned.
    */
    if ( rowSet.isEmpty() )
    {
	if ( cursorStatus == BEFORE )
	{
	    logicalRow = 0;
	    currentRow = null;
	    posStatus = BEFORE;

	    if ( trace.enabled( 3 ) )  trace.write( tr_id + ".load: BEFORE" );
	}
	else
	{
	    logicalRow = 1;
	    currentRow = null;
	    posStatus = AFTER;

	    if ( trace.enabled( 3 ) )  trace.write( tr_id + ".load: AFTER" );
	}
    }
    else
    {
	logicalRow = 1;
	currentRow = (Row)rowSet.get( logicalRow - 1 );
	posStatus = ROW;

	if ( trace.enabled( 3 ) )
	    trace.write( tr_id + ".load: ROW " + currentRow.id +
				 " [" + logicalRow + "]" );
    }

    return;
} // load


/*
** Name: shut
**
** Description:
**	Close the cursor and free associated resources.
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
**	14-May-99 (gordy)
**	    Created.
**	 5-May-00 (gordy)
**	    Extracted base functionality to EdbcRslt and renamed shut().
**	28-Mar-01 (gordy)
**	    Renamed original shut() to closeCursor().  This method now
**	    assumes functionality of close().  Since there is no super-
**	    class method for closeCursor(), make sure cursor is closed
**	    when result-set is closed.
**	19-Jul-01 (gordy)
**	    Reversed the order of calling closeCursor() and super.shut().
**	    The super class shut() resets important global result-set
**	    parameters which are used during closeCursor() to clean-up
**	    the current state of data processing.  
**	 4-Aug-03 (gordy)
**	    Empty row cache and clear row state.
**	22-Sep-03 (gordy)
**	    Replaced BLOB indicator with stream reference.
*/

boolean	// package access
shut()
    throws SQLException
{
    boolean closing;
    
    /*
    ** Local processing requires the current row/cursor
    ** state which is cleared by super-class processing,
    */
    try 
    { 
	/*
	** Flush partial row fetches prior to closing.
	*/
	flush();
	closeCursor(); 
    }
    finally 
    { 
	/*
	** Clear state and do super-class processing.
	*/
	rowSet.clear();
	freeCache.clear();
	logicalRow = rowSet.size() + 1;
	closing = super.shut(); 
    }
    
    return( closing );
} // shut


/*
** Name: fetch
**
** Description:
**	Position and fetch rows.  A simple NEXT operation compatible
**	with all source types is done if equivilent parameters are given.
**	The position reference must be one of BEFORE, ROW, or AFTER.
**
**	Note: Exception trapping/reporting is done by super-class.
**
** Input:
**	reference	Position reference point.
**	offset		Offset from reference.
**	rows		Number of rows to fetch.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 6-Apr-07 (gordy)
**	    Created.
*/

protected void
fetch( int reference, int offset, int rows )
    throws SQLException
{
    short anchor;

    switch( reference )
    {
    case BEFORE : anchor = MSG_POS_BEG;	break;
    case ROW :    anchor = MSG_POS_CUR;	break;
    case AFTER  : anchor = MSG_POS_END;	break;

    /*
    ** An invalid input should probably be handled better
    ** than simply defaulting to NEXT, but the caller is
    ** responsible for ensuring the input is correct.
    */
    default : anchor = MSG_POS_CUR; offset = 1;	break;
    }

    clearResults();
    msg.begin( MSG_CURSOR );
    msg.write( MSG_CUR_FETCH );
    msg.write( MSG_CUR_STMT_ID );
    msg.write( (short)8 );
    msg.write( (int)((stmtID >> 32 ) & 0xffffffff) );
    msg.write( (int)(stmtID & 0xffffffff) );

    /*
    ** Send positioning info if not NEXT.
    */
    if ( anchor != MSG_POS_CUR  ||  offset != 1 )
    {
	msg.write( MSG_CUR_POS_ANCHOR );
	msg.write( (short)2 );
	msg.write( anchor );

	msg.write( MSG_CUR_POS_OFFSET );
	msg.write( (short)4 );
	msg.write( offset );
    }

    /*
    ** Send pre-fetch size if not single row.
    */
    if ( rows != 1 )
    {
	msg.write( MSG_CUR_PRE_FETCH );
	msg.write( (short)4 );
	msg.write( rows );
    }

    msg.done( true );
    readResults();
    return;
} // fetch


/*
** Name: resume
**
** Description:
**	Continue processing query results following column
**	which interrupted processing.  Currently, only
**	LOB columns interrupt result processing, so this
**	method should only be called as a result of a LOB
**	stream closure notification.
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
**	29-Sep-99 (gordy)
**	    Created.
**	15-Nov-99 (gordy)
**	    Bump row count once entire row has been read.
**	13-Dec-99 (gordy)
**	    Complete row is now determined by column count
**	    which must be reset for start of new row.
**	25-Jan-00 (gordy)
**	    Added tracing.
**	19-May-00 (gordy)
**	    Use class lock(), unlock() methods to isolate lock strategy.
**	27-Oct-00 (gordy)
**	    Removed locking (sub-class responsibility).
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 4-Aug-03 (gordy)
**	    Don't need to fudge with row/column counters.  Row counter
**	    now incremented when starting row and column counter not
**	    reset until start of next row.
*/

protected void
resume()
    throws SQLException
{
    /*
    ** Continue processing the current message if any
    ** data remains unread.  Otherwise, the general
    ** message processing routine will proceed to the
    ** next message.
    */
    if ( msg.moreData() )
    {
        if ( trace.enabled( 5 ) )  
    	    trace.write( tr_id + ".resume: continue current message" );

	/*
	** Exit if an interrupt is requested while 
	** processing the current message.  Other-
	** wise, additional messages need to be
	** processed.
	*/
	if ( readData() )  return;
    }

    /*
    ** Now read the remaining messages.
    */
    if ( trace.enabled( 5 ) )  
    	trace.write( tr_id + ".resume: read next message" );
    readResults();
    return;
} // resume


/*
** Name: closeCursor
**
** Description:
**	Close the cursor and free associated resources.
**
**	Caller must ensure that connection is in a suitable state
**	for sending a request to server (sub-classes should over-
**	ride to enforce connection locking and calling methods
**	should flush all active LOB streams).
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
**	 8-Sep-99 (gordy)
**	    Synchronize on DbConn.
**	29-Sep-99 (gordy)
**	    Use DbConn lock()/unlock() methods for synchronization.
**	04-Feb-00 (rajus01)
**	    Reset the counters after flush().
**	 8-Feb-00 (gordy)
**	    Check for connection closed and complete silently.
**	 5-May-00 (gordy)
**	    Extracted base functionality to EdbcRslt and renamed shut().
**	19-May-00 (gordy)
**	    Track closed status.  Use class lock(), unlock() methods to
**	    isolate lock strategy.
**	27-Oct-00 (gordy)
**	    Removed locking (sub-class responsibility).
**	28-Mar-01 (gordy)
**	    Renamed to closeCursor()
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 4-Aug-03 (gordy)
**	    Caller is now required to flush BLOBs prior to call.
**	20-Aug-03 (gordy)
**	    Check if cursor has been automatically closed by server.
**	10-May-05 (gordy)
**	    Don't throw exception when server has already closed.
*/

protected void
closeCursor()
    throws SQLException
{
    synchronized( this )
    {
	if ( cursorStatus == CLOSED )  return;	// Cursor already closed.

	/*
	** The cursor should be closed only when the 
	** result-set is being closed or the cursor has
	** reached EOD (for non-scrollable cursors).
	** For both of these cases, no other state info
	** needs to be updated other than setting the
	** cursor status.
	*/
	cursorStatus = CLOSED;
    }

    /*
    ** Connection may have been closed due to an error.
    ** This is not likely to happen, but we really don't
    ** want to throw that type of exception when just
    ** closing the cursor.
    */
    if ( msg.isClosed() )  return;	// Nothing to do.
    if ( trace.enabled( 3 ) )  trace.write( tr_id + ": closing cursor" );

    try
    {
	msg.begin( MSG_CURSOR );
	msg.write( MSG_CUR_CLOSE );
	msg.write( MSG_CUR_STMT_ID );
	msg.write( (short)8 );
	msg.write( (int)((stmtID >> 32 ) & 0xffffffff) );
	msg.write( (int)(stmtID & 0xffffffff) );
	msg.done( true );
	readResults();
    }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ".shut(): error closing cursor" );
	    SqlExFactory.trace( ex, trace );
	}
	
	/*
	** Ignore error if cursor already closed in server.
	*/
	if ( ex.getErrorCode() != E_GC4811_NO_STMT )  throw ex;
    }

    if ( trace.enabled( 3 ) )  trace.write( tr_id + ": cursor closed!" );
    return;
} // closeCursor


/*
** Name: streamClosed
**
** Description:
**	Stream closure notification method for the StreamListener
**	interface.
**
**	Only one active stream is permitted and it resulted in an
**	interrupt of column processing an a partial row load.
**	Continue processing columns now that the stream has been
**	cleared from the data message queue.
**
** Input:
**	stream	SQL Stream which closed.
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
**	22-Sep-03 (gordy)
**	    Renamed to implemented SQL stream interface.
**	    Replaced BLOB indicator with stream reference.
*/

public void
streamClosed( SqlStream stream )
{
    /*
    ** The only active stream should be the last column loaded,
    ** but make sure this is indeed the case before continueing
    ** with column processing.
    */
    if ( activeStream == null  ||  activeStream != stream )
    {
	if ( trace.enabled( 1 ) )	// Should not happen!
	    trace.write( tr_id + ": invalid LOB stream closure!" );
	return;
    }
    
    if ( trace.enabled( 4 ) )  trace.write( tr_id + ": LOB stream closed" );

    try
    {
	/*
	** Clear stream state and continue column processing.
	*/
	activeStream = null;
	resume();
	checkResults( true );
    }
    catch( SQLException ex )
    {
	/*
	** The caller of the Stream.Listener interface does
	** not care about errors we may hit, so we can only
	** trace the exception and close the result-set.
	** Errors aren't expected except in catastrophic
	** circumstances, so the following should be OK.
	*/
	if ( trace.enabled( 1 ) )  
	{
	    trace.log( tr_id + ": error loading remainder of row" );
	    SqlExFactory.trace( ex, trace );
	}

	try { shut(); }  catch( SQLException ignore ) {}
    }

    return;
} // streamClosed


/*
** Name: flush
**
** Description:
**	Flush the input message stream if called while query 
**	results have not been completely received.
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
**	29-Sep-99 (gordy)
**	    Created.
**	13-Dec-99 (gordy)
**	    Don't need to check column counters, they are
**	    handled elsewhere.
**	27-Oct-00 (gordy)
**	    Clear the BLOB flag if an exception is thrown.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	22-Sep-03 (gordy)
**	    Replaced BLOB indicator with stream reference which
**	    allows direct access.
*/

protected void
flush()
    throws SQLException
{
    /*
    ** Query results are partially read only when a LOB
    ** column interrupts column loading due to an active
    ** stream requiring processing.  Column processing
    ** continues when the active stream is finished/closed.
    **
    ** The easiest way to flush the input message queue is
    ** close the current active stream.  This will result
    ** in a stream closure notification which continues
    ** column processing via the resume() method.  Column
    ** processing will either complete reading the query
    ** result messages or be interrupted by another LOB
    ** column.  We therefore loop until there is no active
    ** stream.
    **
    ** Some additional over-head will occur loading column 
    ** results that will never be used, but this handles a 
    ** complex situation correctly with little additional 
    ** code.
    */
    while( activeStream != null )  activeStream.closeStream();
    return;
} // flush


/*
** Name: flush
**
** Description:
**	Called to fill a partially populated row with additional
**	column values.  The existing row and column values should
**	not be changed.  Columns above those currently populated,
**	upto and including the requested column, must be loaded.
**	Columns beyond the requested column may be loaded but are
**	not required.  
**
**	The contents of LOB columns preceding the requested column
**	are discarded.
**
** Input:
**	index	Internal column index.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 5-May-00 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 4-Aug-03 (gordy)
**	    Renamed and made private (connection locking no longer
**	    required at this level).
**	22-Sep-03 (gordy)
**	    Replaced BLOB indicator with stream reference which
**	    allows direct access.
*/

protected void
flush( int index )
    throws SQLException
{
    /*
    ** A column is not loaded either because there is a
    ** preceding active LOB or the row was not available
    ** (deleted or simply not requested).  In the latter
    ** case, column access is not permitted.
    **
    ** 
    ** When there is an active LOB stream, closing the stream 
    ** will result in a stream closure notification and column
    ** processing will be resumed.  Additional preceding LOB 
    ** columns will interrupt processing, so we loop closing 
    ** active streams until the requested column has been 
    ** loaded.
    */
    while( ! rowSet.isEmpty()  &&  index >= ((Row)rowSet.lastElement()).count )
	if ( activeStream != null )
	    activeStream.closeStream();
	else
	    throw SqlExFactory.get( ERR_GC4021_INVALID_ROW ); 

    return;
} // flush


/*
** Name: setProcResult
**
** Description:
**	Overrides DrvObj method to redirect the procedure 
**	return value to the associated statement object.
**
**	Result-sets have no way to handle this value, but
**	we will receive it if this is a BYREF parameter
**	or row-producing procedure result-set.  The class
**	which implements the CallableStatement interface
**	can process this value and our associated statement
**	will be of that type for the applicable result-sets.
**	So we override the DrvObj method of our super-class
**	to pass the value to the corresponding method in our 
**	associated statement object.
**
** Input:
**	retVal		Procedure return value.
**
** Output:
**	None.
**
** Returns:
**	Void.
**
** History:
**      29-Aug-01 (loera01) SIR 105641
**	    Created.
**	 6-Oct-03 (gordy)
**	    Moved here from RsltProc class (now obsolete).
*/

protected void
setProcResult( int retVal )
    throws SQLException
{
   if ( trace.enabled( 3 ) )  
       trace.write( tr_id + ".setProcResult(" + retVal + ")" );
   stmt.setProcResult( retVal );
   return;
} // setProcResult


/*
** Name: readData
**
** Description:
**	Read data from server.  Overrides default method in DrvObj.
**
**	Guarantees that partial rows are only loaded when required due
**	to LOB column being loaded.
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
**	14-May-99 (gordy)
**	    Created.
**	13-Dec-99 (gordy)
**	    Read multiple rows if complete row received and there is
**	    additional space in the data set.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 4-Aug-03 (gordy)
**	    Moved column loading to readColumns() and row cache
**	    handling to loadRowCache().  Simplified row handling
**	    when pre-fetch disabled.
**	22-Sep-03 (gordy)
**	    Added method to handle row and column data value allocation.
**	20-Jul-07 (gordy)
**	    New row-set representation.
*/

protected boolean
readData()
    throws SQLException
{
    boolean	interrupt = false;
    int		count = 0;

    while( msg.moreData()  &&  ! interrupt )
    {
	/*
	** Row-set may be in one of the following states:
	**    * Empty.
	**    * Last row fully loaded.
	**    * Last row partially loaded.
	**
	** Continue using the current active row (last in the
	** row-set) when partially loaded.  Otherwise, add a 
	** new row to the row-set.
	*/
	if ( rowSet.isEmpty()  ||  
	     ((Row)rowSet.lastElement()).count >= rsmd.count )
	{
	    if ( rowSet.isEmpty()  &&  trace.enabled( 3 ) )
		trace.write( tr_id + ".load: fill row-set starting with row " +
				     cursorRow );
	    createRow( rsmd );
	}

	Row row = (Row)rowSet.lastElement();
	int start = row.count;
	interrupt = readColumns( rsmd, row );

	/*
	** A partial row load should only occur due to 
	** a LOB column interrupting the load process.
	*/
	if ( row.count < rsmd.count  &&  ! interrupt )
	{
	    if ( trace.enabled( 1 ) )
		trace.write( tr_id + ": failed to load entire row" );
	    throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	}

	/*
	** Full row loads are counted for summary trace.
	** Partial row loads are individually traced.
	*/
	if ( start < 1  &&  row.count >= rsmd.count )
	    count++;	// loaded entire row
	else  if ( trace.enabled( 3 ) )
	    trace.write( tr_id + ".load: read " + (row.count - start) +
			 " of " + rsmd.count + " columns starting with " + 
			 (start + 1) );
    }

    if ( count > 0  &&  trace.enabled( 3 ) )
	trace.write( tr_id + ".load: Loaded " + count + " row" +
			     ((count == 1) ? "" : "s") );

    return( interrupt );
} // readData


/*
** Name: rowSetAtBOD
**
** Description:
**	Returns an indication that BOD is included in the current row-set.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean		TRUE if row-set includes BOD.
**
** History:
**	20-Jul-07 (gordy)
**	    Created.
*/

private boolean
rowSetAtBOD()
{
    boolean bod = false;

    /*
    ** A cursor position at BOD, an empty result-set, or  
    ** a row-set including the first row of the result-set 
    ** are assumed to include the BOD position.
    */
    if ( cursorStatus == BEFORE )
	bod = true;				// Physical cursor at BOD
    else  if ( rowSet.size() < 1 )
    {
	/*
	** Since the cursor is not at BOD and the row-
	** set is empty, the cursor must be at EOD.
	*/
	if ( cursorRow == 1 )  bod = true;	// Empty result-set
    }
    else  if ( (((Row)rowSet.get( 0 )).status & Row.FIRST) != 0 )
	bod = true;				// Row-set includes first row

    return( bod );
} // rowSetAtBOD


/*
** Name: rowSetAtEOD
**
** Description:
**	Returns an indication that EOD is included in the current row-set.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean		TRUE if row-set includes EOD.
**
** History:
**	20-Jul-07 (gordy)
**	    Created.
*/

private boolean
rowSetAtEOD()
{
    boolean eod = false;

    /*
    ** A cursor position at EOD or a row-set including the last row
    ** of the result-set are assumed to include the EOD position.
    **
    ** Note: for an empty result-set, cursor would be at EOD.
    */
    if ( cursorStatus == AFTER  ||  cursorStatus == CLOSED )
	eod = true;				// Physical cursor at EOD
    else  if ( rowSet.size() > 0  && 
	       (((Row)rowSet.lastElement()).status & Row.LAST) != 0 )
	eod = true;				// Row-set includes last row

    return( eod );
} // rowSetAtEOD


/*
** Name: clearRowset
**
** Description:
**	Move resources from row-set to free cache.
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
**	20-Jul-07 (gordy)
**	    Created.
*/

private void
clearRowset()
{
    synchronized( this )
    {
	/*
	** Move rows to the free cache.  Work from end of
	** the row-set to minimize overhead.
	*/
	for( int index = rowSet.size() - 1; index >= 0; index-- )
	    freeCache.push( rowSet.remove( index ) );
    }

    logicalRow = 0;
    return;
} // clearRowset


/*
** Name: rowInRowset
**
** Description:
**	Determine if referenced row is in the current row-set.
**	Row is loaded if present.
**
** Input:
**	reference	BEFORE, ROW, or AFTER.
**	offset		Offset from reference.
**
** Output:
**	None.
**
** Returns:
**	boolean		True if row exists in row-set.
**
** History:
**	20-Jul-07 (gordy)
**	    Created.
**	 9-Aug-10 (gordy)
**	    A current row reference requires a physical refresh.
*/

private boolean
rowInRowset( int reference, int offset )
{
    boolean	found = false;

    if ( trace.enabled( 5 ) )
	trace.write( tr_id + ": row cache ref=" + reference + 
			     ", off=" + offset + ", size=" + rowSet.size() + 
			     ", log=" + logicalRow + ", stat=" + cursorStatus +
			     ", row=" + cursorRow );

    /*
    ** Generally, check to see if the target row is in the current
    ** row-set.  It is also possible to detect references to BOD
    ** and EOD, but only if these positions are associated with
    ** the current row-set.
    */
    switch( reference )
    {
    case BEFORE :
    {
	/*
	** Offset is the target row number.
	*/
	if ( offset < 1 )
	{
	    /*
	    ** Target is BOD.  Position to BOD if associated
	    ** with current row-set.
	    */
	    if ( rowSetAtBOD() )
	    {
		logicalRow = 0;
		currentRow = null;
		posStatus = BEFORE;
		found = true;
	    }

	    break;
	}

	if ( rowSet.size() < 1 )
	{
	    /*
	    ** With a positive offset and an empty row-set,
	    ** the only case of interest is EOD.  The logical
	    ** position may move to EOD only if EOD is included
	    ** with the current row-set and the offset is not
	    ** less than the current cursor position (which
	    ** must be at EOD).
	    */
	    if ( rowSetAtEOD()  &&  offset >= cursorRow )
	    {
		logicalRow = rowSet.size() + 1;
		currentRow = null;
		posStatus = AFTER;
		found = true;
	    }

	    break;
	}

	/*
	** The primary case at this point is if the target
	** row is in the row-set, but it is also possible
	** to detect a reference to EOD.
	**
	** Determine the range of rows contained in the row-set.
	*/
	int begin = ((Row)rowSet.get(0)).id;
	int end = begin + rowSet.size() - 1;

	if ( offset > end )  
	{
	    /*
	    ** Target is past end of row-set.  Position
	    ** to EOD if associated with current row-set.
	    */
	    if ( rowSetAtEOD() )
	    {
		logicalRow = rowSet.size() + 1;
		currentRow = null;
		posStatus = AFTER;
		found = true;
	    }

	    break;
	}

	if ( offset >= begin )
	{
	    /*
	    ** Target is in current row-set.
	    */
	    logicalRow = offset - begin + 1;
	    currentRow = (Row)rowSet.get( logicalRow - 1 );
	    posStatus = ROW;
	    found = true;
	}
	break;
    }
    case ROW :
	/*
	** Target row is offset from logical row position.
	*/
	if ( offset <= -logicalRow )
	{
	    /*
	    ** Target is before start of current row-set.  Position 
	    ** to BOD if associated with current row-set.
	    */
	    if ( rowSetAtBOD() )
	    {
		logicalRow = 0;
		currentRow = null;
		posStatus = BEFORE;
		found = true;
	    }

	    break;
	}

	if ( offset > (rowSet.size() - logicalRow) )
	{
	    /*
	    ** Target is past end of current row-set.  Position
	    ** to EOD if associated with current row-set.
	    */
	    if ( rowSetAtEOD() )
	    {
		logicalRow = rowSet.size() + 1;
		currentRow = null;
		posStatus = AFTER;
		found = true;
	    }

	    break;
	}

	/*
	** Target is in current row-set.
	**
	** A current row refresh (offset of 0) requires a
	** physical fetch of the current row rather than 
	** just using the cached image.
	*/
	if ( offset != 0 )
	{
	    logicalRow += offset;
	    currentRow = (Row)rowSet.get( logicalRow - 1 );
	    posStatus = ROW;
	    found = true;
	}
	break;

    case AFTER :
	/*
	** Reverse offset from end of result-set.
	** Check trivial EOD case first.
	*/
	if ( offset >= 0 )
	{
	    /*
	    ** Target is EOD.  Position to EOD if associated
	    ** with current row-set.
	    */
	    if ( rowSetAtEOD() )
	    {
		logicalRow = rowSet.size() + 1;
		currentRow = null;
		posStatus = AFTER;
		found = true;
	    }

	    break;
	}

	/*
	** If the actual result-set size is not known, then
	** there isn't a way to resolve a reference to the 
	** current row-set.
	*/
	if ( rowCount < 0 )  break;

	/*
	** Translate offset to target row number.
	*/
	int target = rowCount + offset + 1;

	if ( target < 1 )
	{
	    /*
	    ** Target is BOD.  Position to BOD if associated
	    ** with current row-set.
	    */
	    if ( rowSetAtBOD() )
	    {
		logicalRow = 0;
		currentRow = null;
		posStatus = BEFORE;
		found = true;
	    }

	    break;
	}

	if ( rowSet.size() > 0 )
	{
	    /*
	    ** Determine the range of rows contained in the row-set.
	    */
	    int begin = ((Row)rowSet.get(0)).id;
	    int end = begin + rowSet.size() - 1;

	    if ( target >= begin  &&  target <= end )
	    {
		/*
		** Target is in current row-set.
		*/
		logicalRow = target - begin + 1;
		currentRow = (Row)rowSet.get( logicalRow - 1 );
		posStatus = ROW;
		found = true;
	    }
	}
	break;
    }

    if ( found  &&  trace.enabled( 3 ) ) 
	switch( posStatus )
	{
	case BEFORE :
	    trace.write( tr_id + ".load: BEFORE" );
	    break;

	case ROW :
	    trace.write( tr_id + ".load: ROW " + currentRow.id + 
				 " [" + logicalRow + "]" );
	    break;

	case AFTER :
	    trace.write( tr_id + ".load: AFTER" );
	    break;
	}

    /*
    ** If the physical cursor has been closed, any 
    ** physical fetch request is assumed to hit EOD.
    */
    if ( ! found  &&  cursorStatus == CLOSED )
    {
	logicalRow = rowSet.size() + 1;
	currentRow = null;
	posStatus = AFTER;
	found = true;
	
	if ( trace.enabled( 3 ) )  
	    trace.write( tr_id + ".load: AFTER (closed)" );
    }

    return( found );
}


/*
** Name: createRow
**
** Description:
**	Create a new row for a result-set.  Cached rows are used
**	when available.  Otherwise, a row is allocated and the
**	column data array is populated with SqlData objects for
**	each column according to the column data type.  Row is
**	appended to current row-set.
**
** Input:
**	rsmd	Result-set Meta-data.
**
** Output:
**	None.
**
** Returns:
**	Row	New row.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	10-May-01 (gordy)
**	    The character datatypes (CHAR, VARCHAR, LONGVARCHAR) may now
**	    be sent as UCS2 character arrays in addition to the existing
**	    Ingres Character Set strings.  The DBMS datatype is used to
**	    distinguish the transport format.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 4-Aug-03 (gordy)
**	    Extracted from readData() to separate row and column processing.
**	22-Sep-03 (gordy)
**	    Changed to use SQL Data objects to hold column values.
**	15-Mar-04 (gordy)
**	    Support BIGINT columns.
**	16-Jun-06 (gordy)
**	    ANSI Date/Time data type support.
**	15-Nov-06 (gordy)
**	    LOB Locator support via Blob/Clob.
**	26-Feb-07 (gordy)
**	    Support buffered LOBs.
**	20-Jul-07 (gordy)
**	    Use cached rows when available.
**	12-Sep-07 (gordy)
**	    Provide configured empty date replacement value.
**	 9-Sep-08 (gordy)
**	    Set max decimal precision.
**	24-Dec-08 (gordy)
**	    Use date/time formatters associated with connection.
**	26-Oct-09 (gordy)
**	    Support BOOLEAN columns.
**	 9-Aug-10 (gordy)
**	    Set maxRow for highest row created.
*/

private Row
createRow( JdbcRSMD rsmd )
    throws SQLException
{
    Row row;

    /*
    ** Use cached row when available.
    */
    if ( ! freeCache.empty() )
	row = (Row)freeCache.pop();
    else
    {
	/*
	** Allocate new row and populate column data array.
	*/
	row = new Row();
	row.columns = new SqlData[ rsmd.count ];

	for( int col = 0; col < rsmd.count; col++ )
	{
            switch( rsmd.desc[ col ].sql_type )
            {
	    case Types.NULL :
		row.columns[ col ] = new SqlNull();
		break;

            case Types.TINYINT :
		row.columns[ col ] = new SqlTinyInt();
		break;

            case Types.SMALLINT : 
		row.columns[ col ] = new SqlSmallInt(); 
		break;

            case Types.INTEGER :
		row.columns[ col ] = new SqlInt();
		break;

	    case Types.BIGINT :
		row.columns[ col ] = new SqlBigInt();
		break;

            case Types.REAL :
		row.columns[ col ] = new SqlReal();
		break;

            case Types.FLOAT :
            case Types.DOUBLE :
		row.columns[ col ] = new SqlDouble();
		break;

            case Types.NUMERIC :
            case Types.DECIMAL :
		row.columns[ col ] = new SqlDecimal( conn.max_dec_prec );
		break;

	    case Types.BOOLEAN :
		row.columns[ col ] = new SqlBool();
		break;

	    case Types.DATE :
		row.columns[ col ] = new SqlDate( conn.dt_frmt );
		break;

	    case Types.TIME :
		row.columns[ col ] = new SqlTime( conn.dt_frmt,
						  rsmd.desc[ col ].dbms_type );
		break;

            case Types.TIMESTAMP :
		switch( rsmd.desc[ col ].dbms_type )
		{
		case DBMS_TYPE_IDATE :
		    row.columns[ col ] = new IngresDate( conn.dt_frmt,
						conn.osql_dates, 
						conn.timeValuesInGMT(),
						conn.cnf_empty_date );
		    break;

		default :
		    row.columns[col] = new SqlTimestamp( conn.dt_frmt,
						rsmd.desc[col].dbms_type );
		    break;
		}
    		break;

            case Types.BINARY :
		row.columns[ col ] = new SqlByte( rsmd.desc[ col ].length );
    		break;

            case Types.VARBINARY :  
		row.columns[ col ] = new SqlVarByte( rsmd.desc[ col ].length );
    		break;

            case Types.CHAR :
    		if ( rsmd.desc[ col ].dbms_type != DBMS_TYPE_NCHAR )
		    row.columns[ col ] = new SqlChar( msg.getCharSet(), 
						rsmd.desc[ col ].length );
    		else
		    row.columns[ col ] = new SqlNChar(
						rsmd.desc[col].length / 2 );
    		break;

            case Types.VARCHAR :    
    		if ( rsmd.desc[ col ].dbms_type != DBMS_TYPE_NVARCHAR )
		    row.columns[ col ] = new SqlVarChar( msg.getCharSet(), 
						rsmd.desc[ col ].length );
    		else
		    row.columns[col] = new SqlNVarChar(
						rsmd.desc[col].length / 2 );
    		break;

            case Types.LONGVARBINARY :
		if ( (conn.cnf_flags & conn.CNF_LOB_CACHE) != 0 )
		    row.columns[col] = new SqlLongByteCache(
						conn.cnf_lob_segSize );
		else
		    row.columns[col] = new SqlLongByte(
					(SqlStream.StreamListener)this );
    		break;

            case Types.LONGVARCHAR :
		if ( (conn.cnf_flags & conn.CNF_LOB_CACHE) != 0 )
		{
		    if ( rsmd.desc[ col ].dbms_type != DBMS_TYPE_LONG_NCHAR )
			row.columns[col] = new SqlLongCharCache( 
							msg.getCharSet(),
							conn.cnf_lob_segSize );
		    else
			row.columns[col] = new SqlLongNCharCache(
							conn.cnf_lob_segSize );
		}
		else
		{
		    if ( rsmd.desc[ col ].dbms_type != DBMS_TYPE_LONG_NCHAR )
			row.columns[ col ] = 
			    new SqlLongChar( msg.getCharSet(), 
					     (SqlStream.StreamListener)this );
		    else
			row.columns[ col ] = 
			    new SqlLongNChar( (SqlStream.StreamListener)this );
		}
   		break;

	    case Types.BLOB :
		row.columns[ col ] = new SqlBLoc( conn );
		break;

	    case Types.CLOB :
		if ( rsmd.desc[ col ].dbms_type != DBMS_TYPE_LNLOC )
	            row.columns[ col ] = new SqlCLoc( conn );
		else
		    row.columns[ col ] = new SqlNLoc( conn );
		break;

            default :
    		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ": unexpected SQL type " + 
					 rsmd.desc[ col ].sql_type );
    		throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
            }
	}
    }

    synchronized( this )
    {
	/*
	** If the row-set is empty, then the new row is the
	** target cursor position.  Otherwise, the cursor
	** position is incremented for the new row.
	*/
	row.id = rowSet.isEmpty() ? cursorRow : ++cursorRow;
	if ( maxRow < row.id )  maxRow = row.id;
	row.count = 0;
	row.status = (row.id == 1) ? Row.FIRST : 0;
	if ( rowCount > 0 && row.id == rowCount )  row.status |= Row.LAST;
	rowSet.add( row );
    }
    return( row );
} // createRow


/*
** Name: readColumns
**
** Description:
**	Read column data from server.  Assumes that the column counter, 
**	column_count, indicates the last column read.  Reads the next
**	column in the row and continues until message is empty , a LOB 
**	column is read or the last column in the row is reached.
**
** Input:
**	rsmd		Row-set Meta-data
**	row		Current row.
**
** Output:
**	None.
**
** Returns:
**	boolean		LOB column interrupted processing.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented BLOB data support.  Changed return type
**	    to permit BLOB stream to interrupt message processing.
**	12-Nov-99 (gordy)
**	    Use configured date formatter.
**	23-Nov-99 (gordy)
**	    Ingres allows for 'empty' dates which format to zero length
**	    strings.  If such a beast is received, create an 'epoch'
**	    timestamp and generate a data truncation warning.
**	30-May-00 (gordy & rajus01)
**	    Fixed the behaviour of select date('today') query.
**	    (Bug #s 101677, 101678).
**	31-Jan-01 (gordy)
**	    Don't throw protocol exceptions for invalid date formats.
**	    EdbcRslt now handles these as conversion errors.
**	 4-Apr-01 (gordy)
**	    Can't create a DataTruncation warning for empty dates here
**	    because row pre-fetching causes warnings for all rows to be
**	    returned on the first row.  Also, a preceding BLOB will
**	    cause the warning to be generated at some unexpected point.
**	18-Apr-01 (gordy)
**	    Use readBytes() method which reads length from input message.
**	10-May-01 (gordy)
**	    The character datatypes (CHAR, VARCHAR, LONGVARCHAR) may now
**	    be sent as UCS2 character arrays in addition to the existing
**	    Ingres Character Set strings.  The DBMS datatype is used to
**	    distinguish the transport format.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	20-Mar-02 (gordy)
**	    JDBC added BOOLEAN data type, treated same as BIT.
**	 4-Aug-03 (gordy)
**	    Extracted from readData() to separate row and column processing.
**	22-Sep-03 (gordy)
**	    Changed to use SQL Data objects to hold column values.
**	    Extracted column data object allocation to allocateRowBuffer().
**	15-Mar-04 (gordy)
**	    Support BIGINT columns.
**	16-Jun-06 (gordy)
**	    ANSI Date/Time data type support.
**	15-Nov-06 (gordy)
**	    LOB Locator support via Blob/Clob.
**	26-Feb-07 (gordy)
**	    Support buffered LOBs.
**	26-Oct-09 (gordy)
**	    Support BOOLEAN columns.
*/

private boolean
readColumns( JdbcRSMD rsmd, Row row )
    throws SQLException
{
    /*
    ** Begin processing columns after last read.
    ** Processing ends after last column in row
    ** is read, or if an interrupt is requested.
    ** Note that the column count (1 based index)
    ** references the next column to load (0 based
    ** index).
    */
    for( ; row.count < rsmd.count; row.count++ )
    {
        int col = row.count;

        /*
	** We only require individual column values, with the
	** exception of LOB segments, to not be split across
	** messages.  If the next column is not available,
	** return without interrupt to allow caller to handle
	** the partial row.
        */
        if ( ! msg.moreData() )  return( false );

        switch( rsmd.desc[ col ].sql_type )
        {
	case Types.NULL :
	    msg.readSqlData( (SqlNull)row.columns[ col ] );
	    break;
	    
        case Types.TINYINT : 
	    msg.readSqlData( (SqlTinyInt)row.columns[ col ] );
    	    break;

        case Types.SMALLINT :	
	    msg.readSqlData( (SqlSmallInt)row.columns[ col ] );
    	    break;

        case Types.INTEGER : 
	    msg.readSqlData( (SqlInt)row.columns[ col ] );
    	    break;

        case Types.BIGINT : 
	    msg.readSqlData( (SqlBigInt)row.columns[ col ] );
    	    break;

        case Types.REAL : 
	    msg.readSqlData( (SqlReal)row.columns[ col ] );
    	    break;

        case Types.FLOAT :
        case Types.DOUBLE :	
	    msg.readSqlData( (SqlDouble)row.columns[ col ] );
    	    break;

        case Types.NUMERIC :
        case Types.DECIMAL : 
	    msg.readSqlData( (SqlDecimal)row.columns[ col ] );
	// TODO: setWarning( new DataTruncation(col + 1, false, true, -1, 0) );
	    break;

	case Types.BOOLEAN :
	    msg.readSqlData( (SqlBool)row.columns[ col ] );
	    break;

	case Types.DATE :
	    msg.readSqlData( (SqlDate)row.columns[ col ] );
	    break;

	case Types.TIME :
	    msg.readSqlData( (SqlTime)row.columns[ col ] );
	    break;

        case Types.TIMESTAMP :
	    switch( rsmd.desc[ col ].dbms_type )
	    {
	    case DBMS_TYPE_IDATE :
		msg.readSqlData( (IngresDate)row.columns[ col ] );
		break;

	    default :
		msg.readSqlData( (SqlTimestamp)row.columns[ col ] );
		break;
	    }
    	    break;

        case Types.BINARY :
	    msg.readSqlData( (SqlByte)row.columns[ col ] );
    	    break;

        case Types.VARBINARY :  
	    msg.readSqlData( (SqlVarByte)row.columns[ col ] );
    	    break;

        case Types.CHAR :
    	    if ( rsmd.desc[ col ].dbms_type != DBMS_TYPE_NCHAR )
		msg.readSqlData( (SqlChar)row.columns[ col ] );
    	    else
		msg.readSqlData( (SqlNChar)row.columns[ col ] );
    	    break;

        case Types.VARCHAR :    
    	    if ( rsmd.desc[ col ].dbms_type != DBMS_TYPE_NVARCHAR )
		msg.readSqlData( (SqlVarChar)row.columns[ col ] );
    	    else
		msg.readSqlData( (SqlNVarChar)row.columns[ col ] );
    	    break;

        case Types.LONGVARBINARY :
	    if ( (conn.cnf_flags & conn.CNF_LOB_CACHE) != 0 )
		msg.readSqlData( (SqlLongByteCache)row.columns[ col ] );
	    else
	    {
    		/*
    		** Initialize BLOB stream.
    		*/
		msg.readSqlData( (SqlLongByte)row.columns[ col ] );
	    
		/*
		** NULL BLOBs don't require special handling.
		** Non-NULL BLOBs interrupt column processing.
		*/
		if ( ! row.columns[ col ].isNull() )
		{
		    activeStream = (SqlStream)row.columns[col];	// Save stream.
    		    row.count++;				// Column done.
    		    return( true );				// Interrupt.
		}
	    }
	    break;

        case Types.LONGVARCHAR :
	    if ( (conn.cnf_flags & conn.CNF_LOB_CACHE) != 0 )
	    {
    		if ( rsmd.desc[ col ].dbms_type != DBMS_TYPE_LONG_NCHAR )
		    msg.readSqlData( (SqlLongCharCache)row.columns[ col ] );
    		else
		    msg.readSqlData( (SqlLongNCharCache)row.columns[ col ] );
	    }
	    else
	    {
    		/*
    		** Initialize CLOB stream.
    		*/
    		if ( rsmd.desc[ col ].dbms_type != DBMS_TYPE_LONG_NCHAR )
		    msg.readSqlData( (SqlLongChar)row.columns[ col ] );
    		else
		    msg.readSqlData( (SqlLongNChar)row.columns[ col ] );
	    
		/*
		** NULL CLOBs don't require special handling.
		** Non-NULL CLOBs interrupt column processing.
		*/
		if ( ! row.columns[ col ].isNull() )
		{
		    activeStream = (SqlStream)row.columns[col];	// Save stream.
    		    row.count++;				// Column done.
    		    return( true );				// Interrupt.
		}
	    }
	    break;

        case Types.BLOB : 
	case Types.CLOB :
	    msg.readSqlData( (SqlLoc)row.columns[ col ] );
    	    break;

        default :
    	    if ( trace.enabled( 1 ) )
    	        trace.write( tr_id + ": unexpected SQL type " + 
				     rsmd.desc[ col ].sql_type );
    	    throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
        }
    }

    return( false );
} // readColumns


/*
** Name: calcTarget
**
** Description:
**	Determines the target cursor position and status info
**	from the original position and positioning criteria.
**
**	If reference is AFTER and actual row count is not
**	known, the calculated row position is just an upper
**	bound.
**
** Input:
**	reference	Position reference.
**	offset		Reference offset.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	20-Jul-07 (gordy)
**	    Created.
*/

private void
calcTarget( int reference, int offset )
{
    /*
    ** If we don't know the total row count, the upper bound 
    ** cannot be determined at this point.  The max integer 
    ** value provides a bound for the upper bound.
    */
    int rows = (rowCount >= 0) ? rowCount : Integer.MAX_VALUE - 1;

    /*
    ** Calculate the target row
    */
    switch( reference )
    {
    case BEFORE : 
	if ( offset <= 0 )
	    updateStatus( BEFORE, 0 );
	else  if ( offset > rows )
	    updateStatus( AFTER, rows + 1 );
	else
	    updateStatus( ROW, offset );
	break;

    case ROW :
	if ( offset <= -cursorRow )
	    updateStatus( BEFORE, 0 );
	else  if ( offset > (rows - cursorRow) )
	    updateStatus( AFTER, rows + 1 );
	else
	    updateStatus( ROW, cursorRow + offset );
	break;

    case AFTER :
	if ( offset < -rows )
	    updateStatus( BEFORE, 0 );
	else  if ( offset >= 0 )
	    updateStatus( AFTER, rows + 1 );
	else
	    updateStatus( ROW, rows + offset + 1 );
	break;
    }

    return;
} // calcTarget


/*
** Name: checkResults
**
** Description:
**	Check result information and update cursor and
**	row-set state accordingly.  Logical cursor status
**	is not changed.
**
** Input:
**	load	Was a row requested to be loaded.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	20-Jul-07 (gordy)
**	    Created.
**	 9-Aug-10 (gordy)
**	    Set maxRow to size of result set if determined.
*/

private void
checkResults( boolean load )
    throws SQLException
{
    /*
    ** If there is an active LOB stream, then no status
    ** information other than the initial cursor state
    ** is available.  The initial settings are assumed
    ** to be valid at this point.
    */
    if ( activeStream != null )  return;

    /*
    ** Check basic status info provided by server.
    **
    ** Note that the cursor may have reached EOD,
    ** but that condition is not detectable if rows
    ** have been loaded and the server does not
    ** provide an explicit indication.  In addition,
    ** similiar exception conditions such as BOD
    ** and DELETED rows are not detectable without
    ** explicit indications.
    */
    if ( (rslt_flags & MSG_RF_CLOSED) != 0 )
    {
	/*
	** Cursor has been automatically closed when it 
	** reached EOD.  If rows have been loaded, the 
	** cursor row references the last row in the 
	** row-set and needs to be updated.  Otherwise, 
	** the target row is assumed to be correct.
	*/
	if ( trace.enabled( 3 ) )
	    trace.write( tr_id + ": cursor automatically closed" );

	updateStatus( CLOSED );

	/*
	** Even though the cursor is actually closed, we still
	** need to call closeCursor() to inform sub-classes
	** of the change in status.  The implementation of
	** closeCursor() in this class will handle the already
	** closed state.  There should not be a need to flush
	** the row-set since the result flag is not received
	** until after all LOBs have been processed.
	*/
	try { closeCursor(); } catch( SQLException ignore ) {}
    }
    else  if ( (rslt_flags & MSG_RF_EOD) != 0 )
    {
	/*
	** If rows have been loaded, the cursor row references 
	** the last row in the row-set and needs to be updated.  
	** Otherwise, the target row is assumed to be correct.
	*/
	if ( trace.enabled( 3 ) )  trace.write( tr_id + ": cursor @ EOD" );

	updateStatus( AFTER );

	/*
	** Explicit EOD is only signalled for non-scrollable 
	** cursors.  Close the cursor since it cannot be used 
	** further.  There should not be a need to flush the
	** row-set since the result flag is not received until
	** after all LOBs have been processed.
	*/
	try { closeCursor(); } catch( SQLException ignore ) {}
    }

    /*
    ** Check specific cursor status and position info.
    */
    if ( (rslt_items & RSLT_ROW_STAT) != 0 )
    {
	int status = ROW;

	/*
	** BEFORE and/or AFTER and/or DELETED should not be set 
	** at the same time.  Give precedence to AFTER, somewhat 
	** arbitrarily, since it is applicable to non-scrollable 
	** cursors.  If the cursor is on a DELETED row, a row 
	** is added to the row-set to represent the deleted row.
	*/
	if ( (rslt_val_rowstat & MSG_RPV_ROW_AFTER) != 0 )
	    status = AFTER;
	else  if ( (rslt_val_rowstat & MSG_RPV_ROW_BEFORE) != 0 )
	    status = BEFORE;
	else  if ( (rslt_val_rowstat & MSG_RPV_ROW_DELETED) != 0 )
	{
	    createRow( rsmd );

	    if ( trace.enabled( 3 ) )
		trace.write( tr_id + ".load: Extend row-set for deleted row " +
				     cursorRow );
	}

	/*
	** The only two valid cases where the cursor can be
	** positioned on a row but no row is returned is for
	** a DELETED row or if the requested row count was 0.
	** The deleted row case was handled above.  If the
	** row-set is still empty, add a row to represent the
	** missing row.  When no rows are requested, no sub-
	** sequent row access should be done.  The column 
	** count is not updated, so any attempted access to 
	** the row will be detected.
	*/
	if ( rowSet.isEmpty()  &&  status == ROW )
	{
	    /*
	    ** If a row was requested, then the only valid reason
	    ** to be positioned on a row without returning the row
	    ** is because it is deleted.  The status info received
	    ** is contradictory.  Best we can do is simply mark the
	    ** row as unaccessible.
	    */
	    if ( load  &&  trace.enabled( 1 ) )
		trace.write( tr_id + ": No row returned when requested (0x" +
			     Integer.toHexString( rslt_val_rowstat ) + ")" );

	    if ( trace.enabled( 3 ) )
		trace.write( tr_id + ".load: Extend row-set for missing row " +
				     cursorRow );

	    createRow( rsmd );
	}

	/*
	** Cursor status and position should be received
	** together, but try to apply the indicated status
	** to the target/current position if not.
	*/
	if ( (rslt_items & RSLT_ROW_POS) != 0 )
	    updateStatus( status, rslt_val_rowpos );
	else
	    updateStatus( status );

	/*
	** Update row status when positioned on last row
	** in the row-set (row-set should never be empty
	** when positioned on a row at this point (see
	** above), but make sure access is possible).
	*/
	if ( cursorStatus == ROW  &&  ! rowSet.isEmpty() )
	{
	    Row row = (Row)rowSet.lastElement();

	    if ( (rslt_val_rowstat & MSG_RPV_ROW_FIRST) != 0 )
		row.status |= Row.FIRST;

	    if ( (rslt_val_rowstat & MSG_RPV_ROW_LAST) != 0 )
		row.status |= Row.LAST;

	    if ( (rslt_val_rowstat & MSG_RPV_ROW_INSERTED) != 0 )
		row.status |= Row.INSERTED;

	    if ( (rslt_val_rowstat & MSG_RPV_ROW_UPDATED) != 0 )
		row.status |= Row.UPDATED;

	    if ( (rslt_val_rowstat & MSG_RPV_ROW_DELETED) != 0 )
		row.status |= Row.DELETED;
	}
    }
    else  if ( (rslt_items & RSLT_ROW_POS) != 0 )
    {
	/*
	** Cursor status and position should be recieved
	** together, but try to apply the indicated position
	** to the target/current status.
	*/
	updateRow( rslt_val_rowpos );
    }

    /*
    ** Failure to load any row can be caused by reaching BOD, 
    ** EOD, a DELETED row, or because no row was requested.  
    ** These conditions cannot be distinguished without 
    ** additional info which would have been processed above.  
    ** It is the servers responsibility to identify deleted 
    ** rows.  If at this point a row was expected, then EOD 
    ** must be assumed.  If we know we aren't at EOD, then
    ** the row must simply be marked as unaccessible.
    */
    if ( rowSet.isEmpty()  &&  cursorStatus == ROW )
    {
	if ( rowCount < 0  ||  cursorRow > rowCount )
	    updateStatus( AFTER );
	else
	{
	    if ( trace.enabled( 3 ) )
		trace.write( tr_id + ".load: Extend row-set for missing row " +
				     cursorRow );
	    createRow( rsmd );
	}
    }

    /*
    ** Last row in row-set is also last row in 
    ** result-set when cursor has reached EOD.
    */
    if ( rowSetAtEOD()  &&  ! rowSet.isEmpty() )
	((Row)rowSet.lastElement()).status |= Row.LAST;

    /*
    ** At this point, the cursor info is as accurate as
    ** we can make it.  Update result-set size if it can 
    ** be determined from the current cursor info.
    */
    if ( (rslt_items & RSLT_ROW_POS) != 0 )
    {
	int size = -1;

	switch( cursorStatus )
	{
	case ROW :
	    if ( rowSet.size() > 0  &&  (
	         ((Row)rowSet.lastElement()).status & Row.LAST) != 0 )
		size = cursorRow;
	    break;

	case AFTER :
	case CLOSED :
	    size = cursorRow - 1;
	    break;
	}

	if ( size >= 0 )
	{
	    if ( rowCount >= 0  &&  rowCount != size )
	    {
		if ( trace.enabled( 3 ) )  trace.write( tr_id + 
			": unexpectedly changing result-set size from " +
			rowCount + " to " + size );
	    }

	    rowCount = size;
	    maxRow = rowCount;
	}
    }

    return;
} // checkResults


/*
** Name: updateRow
**
** Description:
**	Set the current cursor row without changing status.
**	Row-set is updated to reflect correct relationship
**	to cursor position.
**
** Input:
**	row	Cursor row.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	20-Jul-07 (gordy)
**	    Created.
*/

private void
updateRow( int row )
{
    synchronized( this )
    {
	cursorRow = row;
	updateStatus();
    }

    return;
} // updateRow


/*
** Name: updateStatus
**
** Description:
**	Set the current cursor row without changing status.
**	Row-set is updated to reflect correct relationship
**	to cursor position.
**
** Input:
**	row	Cursor row.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	20-Jul-07 (gordy)
**	    Created.
**	 9-Aug-10 (gordy)
**	    Set maxRow to highest row seen.
*/

private void
updateStatus()
{
    /*
    ** Determine row ID of first row in row-set based
    ** on cursor position and whether it is positioned
    ** on the last row or at EOD.
    */
    int rowID = cursorRow - rowSet.size();
    if ( cursorStatus == ROW )  rowID++;

    if ( rowSet.size() > 0  &&  ((Row)rowSet.get( 0 )).id != rowID )
    {
	if ( trace.enabled( 3 ) )  
	    trace.write( tr_id + ": Row-set positioned at row " + rowID );

	for( int index = 0; index < rowSet.size(); index++ )
	{
	    Row row = (Row)rowSet.get( index );
	    row.id = rowID + index;
	    if ( maxRow < row.id )  maxRow = row.id;
	}
    }
    return;   
} // updateStatus


/*
** Name: updateStatus
**
** Description:
**	Set the current cursor status without changing row.
**	Row-set is updated to reflect correct relationship
**	to cursor position.
**
** Input:
**	status	Cursor status.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	20-Jul-07 (gordy)
**	    Created.
*/

private void
updateStatus( int status )
{
    synchronized( this )
    {
	/*
	** Nothing to do if already closed.
	*/
	if ( cursorStatus != CLOSED )
	{
	    /*
	    ** Cursor row must be incremented if previously 
	    ** positioned on (last) row in row-set and now
	    ** at EOD.  Otherwise, current row is assumed
	    ** to be correct.
	    */
	    if ( (status == AFTER  ||  status == CLOSED)  &&
		 cursorStatus == ROW  &&  rowSet.size() > 0 )
	        cursorRow++;

	    cursorStatus = status;
	    updateStatus();
	}
    }

    return;
} // updateStatus


/*
** Name: updateStatus
**
** Description:
**	Set the current cursor status and row.  Row-set is 
**	updated to reflect correct relationship to cursor 
**	position.
**
** Input:
**	status	Cursor status.
**	row	Cursor row.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	20-Jul-07 (gordy)
**	    Created.
*/

private void
updateStatus( int status, int row )
{
    synchronized( this )
    {
	if ( cursorStatus != CLOSED )  cursorStatus = status;
	cursorRow = row;
	updateStatus();
    }

    return;
} // updateStatus


} // class RsltFtch

