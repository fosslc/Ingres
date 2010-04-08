/*
** Copyright (c) 1999, 2009 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: JdbcRslt.java
**
** Description:
**	Defines an abstract base class which implements the JDBC ResultSet
**	interface.
**
**  Classes:
**
**	JdbcRslt	Default JDBC ResultSet implementation.
**	Row		Represents a row in the ResultSet.
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
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	20-Mar-03 (gordy)
**	    Upgraded to JDBC 3.0.
**	15-Apr-03 (gordy)
**	    Encapsulated date/time formatting/parsing.
**	21-Jul-03 (gordy)
**	    Date only values no longer return time from getString() to
**	    match handling of empty dates.  DataTruncation warnings now
**	    have expected length and actual lengths.  Calendars handled
**	    more generically after connection dependent processing done.
**	 4-Aug-03 (gordy)
**	    Moved row pre-fetch handling to sub-classes.
**	22-Sep-03 (gordy)
**	    Column data values now stored in SQL data objects.
**	 1-Dec-03 (gordy)
**	    Changes to support updatable result-set sub-class.
**	28-Apr-04 (wansh01)
**	    Changed updateObject( ) so RsltUpd.updateObject()  
**	    can properly override these methods. 
**	14-Sep-05 (gordy)
**	    Method isLast() returns incorrect result.  Implemented
**	    trivial result-set positioning.  Enhanced BLOB tracing.
**	15-Nov-06 (gordy)
**	    Support LOB Locators via Blob/Clob.
**	 6-Apr-07 (gordy)
**	    Added support for scrollable cursors.
**	 4-May-07 (gordy)
**	    Set class access for reflection.
**	20-Jul-07 (gordy)
**	    Added Row class to encapsulate a ResultSet row.
**      05-Jan-09 (rajus01) SIR 121238
**          - Added new JDBC 4.0 methods to avoid compilation errors with
**            JDK 1.6. The new methods return E_GC4019 error for now.
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to
**            support JDBC 4.0 SQLException hierarchy.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	17-Feb-09 (gordy)
**	    Don't generate truncation warnings for IngresDate values
**	    which are date-only values.
**      7-Apr-09 (rajus01) SIR 121238
**          Implemented new JDBC 4.0 getXX(), updateXX() methods.
*/

import	java.math.BigDecimal;
import	java.net.URL;
import	java.util.Calendar;
import	java.util.TimeZone;
import	java.util.Map;
import	java.io.InputStream;
import	java.io.Reader;
import	java.sql.Statement;
import	java.sql.ResultSet;
import	java.sql.ResultSetMetaData;
import	java.sql.Date;
import	java.sql.Time;
import	java.sql.Timestamp;
import	java.sql.Array;
import	java.sql.Blob;
import	java.sql.Clob;
import  java.sql.NClob;
import  java.sql.SQLXML;
import  java.sql.Struct;
import  java.sql.Wrapper;
import  java.sql.RowId;
import	java.sql.Ref;
import	java.sql.SQLException;
import	java.sql.DataTruncation;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.util.SqlWarn;
import	com.ingres.gcf.util.SqlData;
import	com.ingres.gcf.util.IngresDate;


/*
** Name: JdbcRslt
**
** Description:
**	Abstract JDBC driver base class which implements the JDBC
**	ResultSet interface.  Provides access to column values for
**	current row.  
**
**	A result-set represents a set of N rows which are numbered,
**	if N > 0, 1 to N.  A result-set may be positioned before the
**	first row and after the last row.  An optional rowCount field
**	may be set by a sub-class to indicate the size of the result-
**	set.  If known, the size of the result-set can be used to
**	make some scrolling operations more efficient.
**
**	Sub-classes are responsible for loading data and must implement
**	the load() method which is called to load the next row.  To 
**	optionally support scrollable result-sets, the positioned load() 
**	method may be overridden to read a specific row.  The flush() 
**	method is called prior to performing any row positioning action 
**	and may be overridden to perform any required processing of the 
**	current row.  The shut() method should be overridden if actions 
**	are required when the result-set is closed.
**
**	Sub-classes must set the posStatus field to indicate the current
**	state of the result-set when loading rows.  In addition, if
**	posStatus is set to ROW, the information for the current row
**	must be set in the currentRow field including the row status,
**	row number or id, column count, and column values.  The
**	currentRow field may be null when posStatus is not set to ROW.
**	Sub-classes must instantiate currentRow objects and must be 
**	prepared for the base class to set currentRow to null without 
**	notification.  Sub-classes may assume that these values are 
**	consistent and correct when load() is called and that load() 
**	will only be called when conditions are valid for loading the 
**	requested row.
**
**	Generally, all column values will be loaded when the row is 
**	loaded.  If a sub-class must defer loading a column value, 
**	the column count must be set indicating the subset of available 
**	columns and must also override the flush( column ) method to 
**	permit the base class to request a deferred column to be loaded.  
**
**	Column data is stored as SqlData objects based on the SQL type 
**	of the column.  The following table defines the storage class 
**	used for the supported SQL types.
**
**	SQL Type	Storage Class
**	-------------	--------------------
**	generic NULL	SqlNull
**	BOOLEAN		SqlBool
**	TINYINT		SqlTinyInt
**	SMALLINT	SqlSmallInt
**	INTEGER		SqlInt
**	BIGINT		SqlBigInt
**	REAL		SqlReal
**	DOUBLE		SqlDouble
**	DECIMAL		SqlDecimal
**	CHAR		SqlChar, SqlNChar, SqlString
**	VARCHAR		SqlVarChar, SqlNVarChar, SqlString
**	LONGVARCHAR	SqlLongChar, SqlLongNChar
**	BINARY		SqlByte
**	VARBINARY	SqlVarByte
**	LONGVARBINARY	SqlLongByte
**	DATE		SqlDate
**	TIME		SqlTime
**	TIMESTAMP	IngresDate, SqlTime
**
**	BIT, FLOAT, and NUMERIC are treated the same as BOOLEAN, DOUBLE, 
**	and DECIMAL, respectively.  The character types may be stored as
**	one of several different classes, depending on the source DBMS
**	or Java type.  
**
**	Sub-classes may override columnMap() and getSqlData() if they
**	provide storage for column data objects which are outside the
**	currentRow storage.
**
**  Interface Methods:		(other methods implemented as stubs/covers)
**
**	close			Close the result-set and free resources.
**	beforeFirst		Move before first row in the result-set.
**	first			Move to first row in the result-set.
**	previous		Move to previous row in the result-set.
**	next			Move to next row in the result-set.
**	last			Move to last row in the result-set.
**	afterLast		Move after last row in the result-set.
**	relative		Move to a row relative to current row.
**	absolute		Move to a specific row in the result-set.
**	getStatement		Retrieve associated Statement.
**	getType			Retrieve result-set type.
**	getConcurrency		Retrieve result-set concurrency.
**	getFetchDirection	Retrieve fetch direction.
**	setFetchDirection	Set fetch direction.
**	getFetchSize		Retrieve pre-fetch size.
**	setFetchSize		Set pre-fetch size.
**	getCursorName		Retrieve associated cursor name.
**	getMetaData		Retrieve associated ResultSetMetaData.
**	getRow			Retrieve current row.
**	isBeforeFirst		Is cursor positioned before first row?
**	isAfterLast		Is cursor positioned after last row?
**	isFirst			Is cursor positioned on first row?
**	isLast			Is cursor positioned on last row?
**	rowUpdated		Has current row been updated?
**	rowDeleted		Has current row been deleted?
**	rowInserted		Has current row been inserted?
**	findColumn		Determine column index from column name.
**	wasNull			Determine if last column read was NULL.
**	getBoolean		Retrieve column as a boolean value.
**	getByte			Retrieve column as a byte value.
**	getShort		Retrieve column as a short value.
**	getInt			Retrieve column as a int value.
**	getLong			Retrieve column as a long value.
**	getFloat		Retrieve column as a float value.
**	getDouble		Retrieve column as a double value.
**	getBigDecimal		Retrieve column as a BigDecimal value.
**	getString		Retrieve column as a string value.
**	getBytes		Retrieve column as a byte array value.
**	getDate			Retrieve column as a Date value.
**	getTime			Retrieve column as a Time value.
**	getTimestamp		Retrieve column as a Timestamp value.
**	getBinaryStream		Retrieve column as a binary byte stream.
**	getAsciiStream		Retrieve column as an ASCII character stream.
**	getUnicodeStream	Retrieve column as a Unicode character stream.
**	getCharacterStream	Retrieve column as a Character stream.
**	getObject		Retrieve column as a generic Java object.
**
**  Package Methods:
**
**	shut			Close result-set.
**
**  Protected Constants:
**
**	CLOSED			Result set closed.
**	BEFORE			Positioned prior to first row.
**	AFTER			Positioned after last row.
**	ROW			Positioned on row.
**
**  Protected Data:
**
**	rsmd			Result set meta-data.
**	posStatus		Position status.
**	currentRow		Current row data.
**	rowCount		Number of rows.
**	rs_type			Result-set type.
**	rs_concur		Result-set concurrency.
**	rs_fetch_dir		Fetch direction.
**	rs_fetch_size		Pre-fetch size.
**	rs_max_rows		Max rows to return.
**	rs_max_len		Max column string length.
**
**  Protected Methods:
**
**	load			Load next or specific row.
**	flush			Finish processing current row.
**	columnMap		Validates column index, convert to internal.
**	getSqlData		Retrieve column data value.
**
**  Private Data:
**
**	isNull			Was last column retrieved NULL.
**
**  Private Methods:
**
**	doNext			Load next row.
**	checkRow		Check for available row.
**	columnByName		Determine column index from column name.
**	columnDataValue		Generic column data access.
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
**	31-Oct-02 (gordy)
**	    Added BLOB column helper method getBinary() similiar to other
**	    BLOB methods.  Added checkBlob() to localize common actions.
**	    Renamed closeStrm() to closeBlob().  Dropped stmt field as not 
**	    all sub-classes associated with statements.  Removed instance 
**	    count/id and used super-class instance id.
**	20-Mar-03 (gordy)
**	    Added stubs for UpdateArray(), UpdateBlob(), UpdateClob(),
**	    UpdateRef(), getURL().
**	15-Apr-03 (gordy)
**	    Removed date_ex and used SqlEx directly since this is also
**	    returned by the abstracted date/time parsing methods.
**	 4-Aug-03 (gordy)
**	    Replaced row cache with current row status information.
**	22-Sep-03 (gordy)
**	    Column values stored in SQL data objects which provide
**	    conversion/processing previously provided by this class.
**	22-Sep-03 (gordy)
**	    Column data values now stored in SQL data objects.  Dropped
**	    getBinary(), getAscii(), getUnicode(), getCharacter(),
**	    closeBlob(), checkBlob(), ucs2utf8(), lvc2string(),
**	    lvb2array(), bin2str().  Added getSqlData(), getColByName().
**	    Renamed null_column to isNull.
**	 1-Dec-03 (gordy)
**	    Throw exception for all cursor update methods.  Renamed
**	    getColByName() to columnByName() and made private.  Moved
**	    functionality of getSqlData() to private columnDataValue()
**	    which calls getSqlData() to obtain column SqlData object
**	    from the column data array.  Implemented getBigDecimal()
**	    method with no scale so as to not overload the scale value.
**	 6-Apr-07 (gordy)
**	    Added load() method for positioned fetch.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, permit access.
**	20-Jul-07 (gordy)
**	    Encapsulate row information into Row class and currentRow
**	    field.  Added optional rowCount.
*/

public abstract class
JdbcRslt
    extends	DrvObj
    implements	ResultSet, DrvConst
{

    /*
    ** Status values, only one valid at any time.
    */
    protected static final int	CLOSED	= 0;
    protected static final int	BEFORE	= 1;
    protected static final int	AFTER	= 2;
    protected static final int	ROW	= 3;
    
    /*
    ** Current row information.
    */
    protected JdbcRSMD		rsmd = null;		// ResultSet meta-data.
    protected int		posStatus = BEFORE;
    protected Row		currentRow = null;
    protected int		rowCount = -1;		// Optional

    /*
    ** ResultSet attributes and application settings.
    */
    protected int		rs_type = ResultSet.TYPE_FORWARD_ONLY;
    protected int		rs_concur = ResultSet.CONCUR_READ_ONLY;
    protected int		rs_fetch_dir = ResultSet.FETCH_FORWARD;
    protected int		rs_fetch_size = 0;	// Pre-fetch size
    protected int		rs_max_rows = 0;	// Max rows.
    protected int		rs_max_len = 0;		// Max string length.
    
    /*
    ** Local data.
    */
    private boolean	    isNull = false;    // Was last column NULL?


/*
** Name: JdbcRslt
**
** Description:
**	Class constructor.
**
** Input:
**	conn	    Associated connection.
**	rsmd	    ResultSet meta-data.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**      21-Mar-01 (gordy)
**          Created.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.  Replaced init() with constructor.
**	 4-Aug-03 (gordy)
**	    Removal of row cache allowed for simplification in constructors.
*/

protected
JdbcRslt( DrvConn conn, JdbcRSMD rsmd )
    throws SQLException
{
    super( conn );
    this.rsmd = rsmd;
    title = trace.getTraceName() + "-ResultSet[" + inst_id + "]";
    tr_id = "Rslt[" + inst_id + "]";
    return;
} // JdbcRslt


/*
** Name: load
**
** Description:
**	Position the result-set to the next row and load the column
**	values.  If rows are being cached, this operation applies
**	to the current logical row position and the physical row 
**	position may need to be adjusted accordingly.
**	
**	Sub-classes must implement this method to provide the basic
**	ability to scan forward one row at a time through result-set.
**
**	The posStatus field must be set to indicate the state of the
**	result-set resulting from the operation.
**
**	If the resulting posStatus is ROW, currentRow must be updated 
**	as follows: status to indicate the status of the row, id to 
**	the number of the row in the result-set, columns to the data 
**	values for the row, and count to the number of available 
**	columns.  Loading of column values can be delayed until the 
**	point accessed by overriding the flush( column ) method.
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
*/

protected abstract void load() throws SQLException;


/*
** Name: load
**
** Description:
**	Position the result-set to a specific row and optionally load 
**	the column values.  If rows are being cached, this operation 
**	applies to the current logical row position and the physical 
**	row position may need to be adjusted accordingly.  
**
**	Sub-classes should override this methed to provide scrolling
**	of the result-set.  Implementations should trace any exception 
**	thrown and attempt to shut() the result-set.  The default
**	implementation throws and unsupported operation exception.
**
**	The target row is determined by a reference point and row 
**	offset.  Reference point BEFORE corresponds to before the
**	first row, ROW to the current position, and AFTER to after
**	the last row.  An offset of 0 targets the reference point, 
**	positive offsets target positions following the reference 
**	point, and negative offsets target positions preceding the 
**	reference point.
**	
**	The posStatus field must be set to indicate the state of the
**	result-set resulting from the operation.
**
**	If the resulting posStatus is ROW, currentRow must be updated 
**	as follows: status to indicate the status of the row, id to 
**	the number of the row in the result-set, columns to the data 
**	values for the row, and count to the number of available 
**	columns.  Loading of column values can be delayed until the 
**	point accessed by overriding the flush( column ) method.
**	If position only has been requested, the column count should
**	be set to 0.  Columns should not be accessed in this case,
**	but the flush() methods must be prepared to handle this state.
**
** Input:
**	reference	Reference position.
**	offset		Row offset from reference position.
**	load		True to load row, false to just position.
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
load( int reference, int offset, boolean load )
    throws SQLException
{
    /* Scrolling not supported */
    if ( trace.enabled() )  trace.log( title + ": scrolling not supported" );
    throw( SqlExFactory.get( ERR_GC4019_UNSUPPORTED ) ); 
} // load


/*
** Name: flush
**
** Description:
**	Called prior to any positioning action to complete
**	processing of the current row.
**
**	Sub-classes should override this method if any action
**	is needed to clean-up the state of the current row
**	prior to changing position of the result-set.  Default
**	implementation does nothing.
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
**	 6-Apr-07 (gordy)
*/

protected void
flush()
    throws SQLException
{
    return;	// Nothing to do!
} // flush


/*
** Name: flush
**
** Description:
**	Called to load a column value when loading has been deferred.
**
**	Sub-class should override this method if deferred loading
**	of columns is necessary.  Otherwise, sub-classes must ensure
**	that the column count for the current row is properly set.
**
** Input:
**	index	Internal column index [0,rsmd.count-1].
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

protected void
flush( int index )
    throws SQLException
{
    throw SqlExFactory.get( ERR_GC4011_INDEX_RANGE );
} // flush


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
    if ( posStatus != CLOSED )  try { shut(); } catch( Exception ignore ) {}
    super.finalize();
    return;
} // finalize


/*
** Name: close
**
** Description:
**	Close the result-set.
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
    isNull = false;

    try 
    { 
	if ( ! shut()  &&  trace.enabled( 5 ) )  
	    trace.write( tr_id + ": result-set already closed!" );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )
	    trace.log( title + ".close(): error closing result-set" );
	if ( trace.enabled( 1 ) )  SqlExFactory.trace( ex, trace );
	throw ex;
    }
    return;
} // close


/*
** Name: shut
**
** Description:
**	Internal method for closing the result-set.  Returns an 
**	indication if the result-set had already been closed.
**
**	Sub-classes should override this method to perform their 
**	own shutdown activities.  Default implementation ensures
**	update of result-set status occurs only once.  If over-
**	ridden, this method should still be called (super.shut()) 
**	to set the result-set status info.
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
**	 4-Aug-03 (gordy)
**	    Replaced row cache with status info.
*/

boolean	// package access needed
shut()
    throws SQLException
{
    synchronized( this )
    {
	if ( posStatus == CLOSED )  return( false );
	posStatus = CLOSED;
    }

    if ( trace.enabled( 3 ) )  trace.write( tr_id + ": result-set closed" );

    currentRow = null;
    return( true );
} // shut


/*
** Name: beforeFirst
**
** Description:
**	Position result-set before first row in the result-set.
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
**	14-Sep-05 (gordy)
**	    Implemented for trivial cases where requested position
**	    is known to match current position or request can be
**	    satisfied by calling next().  Otherwise not supported 
**	    at this level.
**	 6-Apr-07 (gordy)
**	    Support scrollable cursors.
*/

public synchronized void 
beforeFirst() 
    throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".beforeFirst()" );

    /*
    ** Clean-up current state.
    */
    flush();
    warnings = null;
    isNull = false;

    /*
    ** Result-set must be open and not at BOD.
    */
    switch( posStatus )
    {
    case BEFORE :
	if ( trace.enabled( 3 ) )
	    trace.write( tr_id + ".beforeFirst: result-set at BOD" );
	break;

    case ROW :
    case AFTER :
	load( BEFORE, 0, false );
	checkRow();
	break;

    default :
	if ( trace.enabled() )
	    trace.log( title + ".beforeFirst: result-set closed" );
	throw SqlExFactory.get( ERR_GC401D_RESULTSET_CLOSED );
    }

    return;
} // beforeFirst


/*
** Name: first
**
** Description:
**	Position result-set at first row in the result-set and returns 
**	an indication if the end of the result-set has been reached.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean		True if row is valid, false if end of result-set.
**
** History:
**	14-Sep-05 (gordy)
**	    Implemented for trivial cases where requested position
**	    is known to match current position or request can be
**	    satisfied by calling next().  Otherwise not supported 
**	    at this level.
**	 6-Apr-07 (gordy)
**	    Support scrollable cursors.
*/

public synchronized boolean 
first() 
    throws SQLException
{ 
    boolean ready = false;

    if ( trace.enabled() )  trace.log( title + ".first()" );

    /*
    ** Clean-up current state.
    */
    flush();
    warnings = null;
    isNull = false;

    /*
    ** Result-set must be open.
    */
    switch( posStatus )
    {
    case BEFORE :
	/*
	** FIRST is not equivilent to NEXT for scrollable cursors.
	** If the result-set is empty, FIRST should position to
	** BEFORE while NEXT should position to AFTER.
	*/
	if ( rs_type == ResultSet.TYPE_FORWARD_ONLY )
	{
	    ready = doNext();		// Request equivilent to next().
	    break;
	}

	/*
	** Fall through for general processing.
	*/

    case ROW :
    case AFTER :
	/*
	** Even if currently positioned on the first row,
	** the first row must be read again (refreshed)
	** to satisfy the semantics of this method.
	*/
	load( BEFORE, 1, true );
	ready = checkRow();
	break;

    default :
	if ( trace.enabled( 3 ) )
	    trace.write( tr_id + ".first: result-set closed" );
	break;
    }

    if ( trace.enabled() )  trace.log( title + ".first: " + ready );
    return( ready );
} // first


/*
** Name: previous
**
** Description:
**	Position result-set at previous row in the result-set and returns 
**	an indication if the beginning of the result-set has been reached.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean		True if row is valid, false if end of result-set.
**
** History:
**	14-Sep-05 (gordy)
**	    Implemented for trivial cases where requested position
**	    is known to match current position or request can be
**	    satisfied by calling next().  Otherwise not supported 
**	    at this level.
**	 6-Apr-07 (gordy)
**	    Support scrollable cursors.
*/

public synchronized boolean 
previous() 
    throws SQLException
{ 
    boolean ready = false;

    if ( trace.enabled() )  trace.log( title + ".previous()" );

    /*
    ** Clean-up current state.
    */
    flush();
    warnings = null;
    isNull = false;

    /*
    ** Result-set must be open and not at BOD.
    */
    switch( posStatus )
    {
    case BEFORE :
	if ( trace.enabled( 3 ) )
	    trace.write( tr_id + ".previous: result-set at BOD" );
	break;

    case ROW :
    case AFTER :
	/*
	** Request the previous row be loaded by sub-class.
	*/
	load( ROW, -1, true );
	ready = checkRow();
	break;

    default :
	if ( trace.enabled( 3 ) )
	    trace.write( tr_id + ".previous: result-set closed" );
	break;
    }

    if ( trace.enabled() )  trace.log( title + ".previous: " + ready );
    return( ready );
} // previous


/*
** Name: refreshRow
**
** Description:
**	Refresh the current row in the result-set.
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
**	14-Sep-05 (gordy)
**	    Implemented for trivial cases where requested position
**	    is known to match current position or request can be
**	    satisfied by calling next().  Otherwise not supported 
**	    at this level.
**	 6-Apr-07 (gordy)
**	    Support scrollable cursors.
*/

public synchronized void
refreshRow() 
    throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".refreshRow()" );

    /*
    ** Clean-up current state.
    */
    flush();
    warnings = null;
    isNull = false;

    /*
    ** Result-set must be open and on a row.
    */
    switch( posStatus )
    {
    case ROW :
	/*
	** Request the current row be loaded by sub-class.
	*/
	load( ROW, 0, true );
	checkRow();
	break;

    case BEFORE :
    case AFTER :
	if ( trace.enabled() )  
	    trace.log( title + "refreshRow: not positioned on a row" );
	throw( SqlExFactory.get( ERR_GC4021_INVALID_ROW ) ); 

    default :
	if ( trace.enabled() )
	    trace.log( title + ".refreshRow: result-set closed" );
	throw SqlExFactory.get( ERR_GC401D_RESULTSET_CLOSED );
    }

    return;
} // refreshRow


/*
** Name: next
**
** Description:
**	Position result-set at next row in the result-set and returns 
**	an indication if the end of the result-set has been reached.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean		True if row is valid, false if end of result-set.
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
**	    Close result-set at end-of-rows or an exception occurs.
**	 5-Sep-00 (gordy)
**	    Don't throw exception if result-set is closed, simply
**	    return the no-data indication.  This behavior matches
**	    ESQL and OpenAPI, as well as the SQL92 standard.
**	 4-Aug-03 (gordy)
**	    Moved pre-fetch row handling to sub-classes.
**	 6-Apr-07 (gordy)
**	    Simulate AFTER when max row limit exceeded.  Only
**	    close result-set in this state when non-scrollable.
*/

public synchronized boolean
next()
    throws SQLException
{
    boolean ready = false;
    
    if ( trace.enabled() )  trace.log( title + ".next()" );

    /*
    ** Clean-up current state.
    */
    flush();
    warnings = null;
    isNull = false;
    
    /*
    ** Result-set must be open, not at EOD, and must be
    ** within max row limit (if set).
    */
    switch( posStatus )
    {
    case BEFORE :
    case ROW :
	ready = doNext();
	break;

    case AFTER :
	if ( trace.enabled( 3 ) )
	    trace.write( tr_id + ".next: result-set at EOD" );
	break;
	
    default :
	if ( trace.enabled( 3 ) )
	    trace.write( tr_id + ".next: result-set closed" );
	break;
    }

    if ( trace.enabled() )  trace.log( title + ".next: " + ready );
    return( ready );
} // next


/*
** Name: last
**
** Description:
**	Position result-set at last row in the result-set and returns 
**	an indication if the end of the result-set has been reached.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean		True if row is valid, false if end of result-set.
**
** History:
**	14-Sep-05 (gordy)
**	    Implemented for trivial cases where requested position
**	    is known to match current position or request can be
**	    satisfied by calling next().  Otherwise not supported 
**	    at this level.
**	 6-Apr-07 (gordy)
**	    Support scrollable cursors.
*/

public synchronized boolean 
last() 
    throws SQLException
{ 
    boolean ready = false;

    if ( trace.enabled() )  trace.log( title + ".last()" );

    /*
    ** Clean-up current state.
    */
    flush();
    warnings = null;
    isNull = false;
    
    /*
    ** Result-set must be open.
    */
    switch( posStatus )
    {
    case BEFORE :
    case ROW :
    case AFTER :
	/*
	** Limit to max rows (if set and less than actual rows).
	*/
	if ( rs_max_rows > 0  &&
	     (rowCount < 0  ||  rs_max_rows < rowCount) )
	{
	    if ( trace.enabled( 3 ) )
	        trace.write( tr_id + 
			     ".last: limit to max row " + rs_max_rows );

	    /*
	    ** If there are fewer than max rows in the result-set,
	    ** resulting position will be EOD.  Otherwise, a valid
	    ** row will be loaded and simulated as last row.
	    */
	    load( BEFORE, rs_max_rows, true );
	    if ( (ready = checkRow()) )  break;	// Simulate as last row

	    /*
	    ** Result-set must have fewer rows than max row limit.
	    ** Need to position to actual last row in result-set.
	    ** Clean-up current state of preceeding load() and
	    ** fall through to do standard positioning.
	    */
	    flush();
	    warnings = null;
	}
	
	/*
	** Position to last row.
	*/
	load( AFTER, -1, true );
	ready = checkRow();
	break;

    default :
	if ( trace.enabled( 3 ) )
	    trace.write( tr_id + ".last: result-set closed" );
	break;
    }

    if ( trace.enabled() )  trace.log( title + ".last: " + ready );
    return( ready );
} // last


/*
** Name: afterLast
**
** Description:
**	Position result-set after last row in the result-set.
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
**	14-Sep-05 (gordy)
**	    Implemented for trivial cases where requested position
**	    is known to match current position or request can be
**	    satisfied by calling next().  Otherwise not supported 
**	    at this level.
**	 6-Apr-07 (gordy)
**	    Support scrollable cursors.
*/

public synchronized void 
afterLast() 
    throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".afterLast()" );

    /*
    ** Clean-up current state.
    */
    flush();
    warnings = null;
    isNull = false;

    /*
    ** Result-set must be open and not at EOD.
    */
    switch( posStatus )
    {
    case BEFORE :
    case ROW :
	/*
	** Limit to max rows (if set and less than actual rows).
	*/
	if ( rs_max_rows > 0  &&
	     (rowCount < 0  ||   rs_max_rows < rowCount) )
	{
	    if ( trace.enabled( 3 ) )
	        trace.write( tr_id + 
			     ".afterLast: limit to max row = " + rs_max_rows );

	    /*
	    ** There may be fewer than max rows in the result
	    ** set, in which case position will be at EOD.  
	    ** Otherwise, EOD will be simulated.  In either 
	    ** case, don't need to load the row, just position.
	    */
	    load( BEFORE, rs_max_rows + 1, false );
	    checkRow();
	    break;
	}

	/*
	** Position at EOD.
	*/
	load( AFTER, 0, false );
	checkRow();
	break;

    case AFTER :	
	if ( trace.enabled( 3 ) )
	    trace.write( tr_id + ".afterLast: result-set at EOD" );
	break;

    default :		
	if ( trace.enabled() )
	    trace.log( title + ".afterLast: result-set closed" );
	throw SqlExFactory.get( ERR_GC401D_RESULTSET_CLOSED );
    }

    return;
} // afterLast


/*
** Name: relative
**
** Description:
**	Position result-set relative to current row in the result-set 
**	and returns an indication if the end of the result-set has been 
**	reached.
**
** Input:
**	offset		Offset from current row position.
**
** Output:
**	None.
**
** Returns:
**	boolean		True if row is valid, false if end of result-set.
**
** History:
**	14-Sep-05 (gordy)
**	    Implemented for trivial cases where requested position
**	    is known to match current position or request can be
**	    satisfied by calling next().  Otherwise not supported 
**	    at this level.
**	 6-Apr-07 (gordy)
**	    Support scrollable cursors.
*/

public synchronized boolean 
relative( int offset ) 
    throws SQLException
{ 
    boolean ready = false;

    if ( trace.enabled() )  trace.log( title + ".relative( " + offset + " )" );
    
    /*
    ** Clean-up current state.
    */
    flush();
    warnings = null;
    isNull = false;

    /*
    ** Grab the row number when available.
    */
    int rowId = (posStatus == ROW) ? currentRow.id : -1;

    switch( posStatus )
    {
    case BEFORE :
	/*
	** Check trivial case, already at BOD.
	*/
	if ( offset <= 0 )
	{
	    if ( trace.enabled( 3 ) )
		trace.write( tr_id + ".relative: result-set at BOD" );
	    break;
	}

	/*
	** Fall through for general case.
	** BOD is equivilent to row 0.
	*/
	rowId = 0;

    case ROW :
	/*
	** Is the request equivilent to a next()?
	*/
	if ( offset == 1 )
	{
	    ready = doNext();
	    break;
	}

	/*
	** Will the max row limit be exceeded
	** (if set and less than actual rows)?
	*/
	if ( 
	     rs_max_rows > 0  &&  
	     (rowCount < 0  ||  rs_max_rows > rowCount)  &&
	     offset > 0  &&  (rowId + offset) > rs_max_rows 
	   )
	{
	    if ( trace.enabled( 3 ) )
		trace.write( tr_id + ".relative: max row limit reached" );

	    /*
	    ** There may be fewer than max rows in the result
	    ** set, in which case position will be at EOD.  
	    ** Otherwise, EOD will be simulated.  In either 
	    ** case, don't need to load the row, just position.
	    */
	    load( BEFORE, rs_max_rows + 1, false );
	    ready = checkRow();
	    break;
	}

	/*
	** Reference relative to current position
	** to handle fall through from preceding case.
	*/
	load( posStatus, offset, true );
	ready = checkRow();
	break;

    case AFTER :
	if ( offset >= 0 )
	{
	    if ( trace.enabled( 3 ) )
		trace.write( tr_id + ".relative: result-set at EOD" );
	    break;
	}

	load( ROW, offset, true );
	ready = checkRow();
	break;

    default :
	if ( trace.enabled() )
	    trace.log( title + ".relative: result-set closed" );
	throw SqlExFactory.get( ERR_GC401D_RESULTSET_CLOSED );
    }

    if ( trace.enabled() )  trace.log( title + ".relative: " + ready );
    return( ready );
} // relative


/*
** Name: absolute
**
** Description:
**	Position result-set to specific row in the result-set and 
**	returns an indication if the end of the result-set has been 
**	reached.  A positive row offset positions relative to start
**	of result-set.  A negative offset positions relative to end
**	of result-set.
**
** Input:
**	offset		Row offset.
**
** Output:
**	None.
**
** Returns:
**	boolean		True if row is valid, false if end of result-set.
**
** History:
**	14-Sep-05 (gordy)
**	    Implemented for trivial cases where requested position
**	    is known to match current position or request can be
**	    satisfied by calling next().  Otherwise not supported 
**	    at this level.
**	 6-Apr-07 (gordy)
**	    Support scrollable cursors.
*/

public synchronized boolean 
absolute( int offset ) 
    throws SQLException
{ 
    boolean ready = false;

    if ( trace.enabled() )  trace.log( title + ".absolute( " + offset + " )" );

    /*
    ** Clean-up current state.
    */
    flush();
    warnings = null;
    isNull = false;

    /*
    ** Grab the row number when available.
    */
    int rowId = (posStatus == ROW) ? currentRow.id : -1;

    switch( posStatus )
    {
    case BEFORE :
	/*
	** Check trivial case, already at BOD.
	*/
	if ( offset == 0 )
	{
	    if ( trace.enabled( 3 ) )
		trace.write( tr_id + ".absolute: result-set at BOD" );
	    break;
	}

	/*
	** Fall through for general case.
	** BOD is equivilent to row 0.
	*/
	rowId = 0;

    case ROW :
	/*
	** Is the request equivilent to a next()?
	*/
	if ( offset > 0  &&  (offset - 1) == rowId )
	{
	    ready = doNext();
	    break;
	}

	/*
	** Fall through for general case.
	*/

    case AFTER :
	/*
	** Will the max row limit be exceeded
	** (if set and less then actual rows)?
	** Negative offsets require special
	** processing.
	*/
	if ( 
	     rs_max_rows > 0  &&
	     (rowCount < 0  ||  rs_max_rows < rowCount)  &&
	     (offset < 0  ||  offset > rs_max_rows)
	   )
	{
	    /*
	    ** Positive values are relative to BOD
	    ** (positive offset exceeds max rows).
	    */
	    if ( offset >= 0 )
	    {
		if ( trace.enabled( 3 ) )
		    trace.write( tr_id + ".absolute: max row limit reached" );

		/*
		** There may be fewer than max rows in the result
		** set, in which case position will be at EOD.  
		** Otherwise, EOD will be simulated.  In either 
		** case, don't need to load the row, just position.
		*/
		load( BEFORE, rs_max_rows + 1, false );
		ready = checkRow();
		break;
	    }

	    /*
	    ** Negative values are relative to EOD, which is the
	    ** smaller of max rows or actual total rows.  If the
	    ** the actual row count is known, then the target row
	    ** can be calculated directly.
	    **
	    ** If we don't know the actual row count, we must first
	    ** try relative to the actual EOD.  If the resulting
	    ** position is beyond the point calculated from the
	    ** max row limit, then an additional positioning is
	    ** required to properly handle the limit condition.
	    */
	    if ( trace.enabled( 3 ) )  trace.write( tr_id + 
			".absolute: adjust offset for max row limit" );

	    /*
	    ** If the offset from the row limit reaches BOD,
	    ** then the same will be true for any actaul row
	    ** count less than the limit.  This case can
	    ** therefore be handled independently.
	    */
	    if ( offset < -rs_max_rows )
	    {
		load( BEFORE, 0, false );
		ready = checkRow();
		break;
	    }

	    /*
	    ** If the actual row count is known, the target
	    ** row can be calculated directly because max
	    ** rows has been verified to be less than actual
	    ** and BOD condition has already been handled.
	    */
	    if ( rowCount > 0 )
	    {
		load( BEFORE, rs_max_rows + offset + 1, true );
		ready = checkRow();
		break;
	    }

	    /*
	    ** Position relative to the actual number of rows.
	    */
	    load( AFTER, offset, true );

	    /*
	    ** If there are more actual rows than the max 
	    ** row limit, the resulting position will be 
	    ** greater than the target determined from the 
	    ** limit.  As a result, the desired target row
	    ** will need to be loaded.  Otherwise, we are 
	    ** at the correct row.
	    */
	    if ( posStatus == ROW )
	    {
		int target = rs_max_rows + offset + 1;

		if ( currentRow.id > target )  
		{
		    /*
		    ** Clean-up current state from prior load().
		    */
		    flush();
		    warnings = null;
		    load( BEFORE, target, true );
		}
	    }

	    ready = checkRow();
	    break;
	}

	/*
	** Don't need to load the row if targeting BOD.
	** Positive values are relative to BOD, negative
	** values are relative to EOD.
	*/
	if ( offset == 0 )
	    load( BEFORE, 0, false );
	else  if ( offset > 0 )
	    load( BEFORE, offset, true );
	else
	    load( AFTER, offset, true );

	ready = checkRow();
	break;

    default :
	if ( trace.enabled() )
	    trace.log( title + ".absolute: result-set closed" );
	throw SqlExFactory.get( ERR_GC401D_RESULTSET_CLOSED );
    }

    if ( trace.enabled() )  trace.log( title + ".absolute: " + ready );
    return( ready );
} // absolute


/*
** Name: doNext
**
** Description:
**	Loads the next row of the result-set for any type
**	of result-set.  May be used to simulate a scrolling
**	operation which is equivilent to a next() for a non-
**	scrolling result-set.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean		True if row was loaded, FALSE otherwise.
**
** History:
**	 6-Apr-07 (gordy)
**	    Created.
*/

private boolean
doNext()
    throws SQLException
{
    /*
    ** Has the max row limit been reached?
    */
    if ( 
	 rs_max_rows > 0  &&  
	 posStatus == ROW  &&  
	 currentRow.id >= rs_max_rows 
       )
    {
	if ( trace.enabled( 3 ) )
	    trace.write( tr_id + ".doNext: max row limit reached" );

	/*
	** For non-scrollable result-sets: the result-set
	** can be closed since no other operation can be
	** done once the (simulated) end-of-data point is
	** reached.
	**
	** For scrollable result-sets: need to position to
	** the row position so that subsequent scrolling
	** will be correct.  Don't need to load row since 
	** EOD will be simulated.
	*/
	if ( rs_type == ResultSet.TYPE_FORWARD_ONLY )
	    try { shut(); }  catch( SQLException ignore ) {}
	else
	    load( BEFORE, rs_max_rows + 1, false );
    }
    else
    {	
	/*
	** Request the next row be loaded by sub-class.
	*/
	try { load(); }
	catch( SQLException ex )
	{
	    if ( trace.enabled() )  
		trace.log( title + ".next(): error loading row" );
	    if ( trace.enabled( 1 ) )  SqlExFactory.trace( ex, trace );
	    try { shut(); }  catch( SQLException ignore ) {}
	    throw ex;
	}
    }

    return( checkRow() );
} // doNext


/*
** Name: checkRow
**
** Description:
**	Check posStatus to determine if row is available.
**	Enforces artificial result-set size.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean		True if row is available, FALSE otherwise.
**
** History:
**	 6-Apr-07 (gordy)
**	    Created.
*/

private boolean
checkRow()
{
    boolean ready = false;

    if ( posStatus == ROW )
    {
	ready = true;

	if ( rs_max_rows > 0 )
	    if ( currentRow.id == rs_max_rows )
		currentRow.status |= Row.LAST;
	    else  if ( currentRow.id > rs_max_rows )
	    {
		posStatus = AFTER;		// Simulate EOD.
		currentRow = null;
		ready = false;
	    }
    }

    return( ready );
} // checkRow

/*
** Name: isClosed 
**
** Description:
**	Retrieves whether this ResultSet object has been closed.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean		True if this ResultSet object is closed; false otherwise.
**
** History:
**	 7-Apr-09 (rajus01) SIR 121238
**	    Created.
*/
public boolean 
isClosed()
throws SQLException
{ return ( ( posStatus == CLOSED ) ? true : false ); }

/*
** Name: getHoldability 
**
** Description:
**	Retrieves the holdability of this ResultSet object.
**  ResultSet.HOLD_CURSORS_OVER_COMMIT is not supported.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	 ResultSet.CLOSE_CURSORS_AT_COMMIT.
**
** History:
**	 7-Apr-09 (rajus01) SIR 121238
**	    Created.
*/
public int 
getHoldability()
throws SQLException
{
   return( ResultSet.CLOSE_CURSORS_AT_COMMIT ); 
}

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
**	    Removed statement info since only some sub-classes are
**	    associated with statements.
*/

public Statement
getStatement()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getStatement(): " + null );
    return( null );
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
**	int	Result-set concurrency.
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
	throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
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
    if ( size < 0  ||  (rs_max_rows > 0  &&  size > rs_max_rows) )
	throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );

    rs_fetch_size = size;
    return;
} // setFetchSize


/*
** Name: getCursorName
**
** Description:
**	Returns NULL.  This method should be overridden by a sub-class
**	which supports cursor result-sets.
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
**	Creates a ResultSetMetaData object describing the result-set.
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
    if( posStatus == CLOSED )  throw SqlExFactory.get( ERR_GC401D_RESULTSET_CLOSED );
    return( rsmd );
} // getMetaData


/*
** Name: getRow
**
** Description:
**	Return the current row number.  Zero returned if cursor
**	is not positioned on a row.
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
    int row = (posStatus != ROW) ? 0 : currentRow.id;
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
**	boolean	    TRUE if before first, FALSE otherwise.
**
** History:
**	 6-Nov-00 (gordy)
**	    Created.
*/

public boolean
isBeforeFirst()
    throws SQLException
{
    boolean val = (posStatus == BEFORE  &&  rowCount != 0);
    if ( trace.enabled() )  trace.log( title + ".isBeforeFirst(): " + val );
    return( val );
} // isBeforeFirst


/*
** Name: isAfterLast
**
** Description:
**	Is cursor after the last row of the result-set.
**
**	This method should also return FALSE if result-set is
**	empty.  Therefore, there must have been at least one 
**	row returned for this method to return TRUE.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    TRUE if after last, FALSE otherwise.
**
** History:
**	 6-Nov-00 (gordy)
**	    Created.
*/

public boolean
isAfterLast()
    throws SQLException
{
    boolean val = (posStatus == AFTER  &&  rowCount != 0);
    if ( trace.enabled() )  trace.log( title + ".isAfterLast(): " + val );
    return( val );
} // isAfterLast


/*
** Name: isFirst
**
** Description:
**	Is cursor on first row of the result-set.
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
    boolean val = (posStatus == ROW  &&  (currentRow.status & Row.FIRST) != 0);
    if ( trace.enabled() )  trace.log( title + ".isFirst(): " + val );
    return( val );
} // isFirst


/*
** Name: isLast
**
** Description:
**	Is cursor on last row of the result-set.
**
**	It is not currently possible to determine if the
**	cursor is on the last row under all conditions.
**	Ingres does not provide cursor positioning info.
**
**	Non-cursor based result-sets can provide position
**	information.  READONLY cursors along with pre-
**	fetching can determine last row status if the last
**	block of rows is not completely filled. Otherwise,
**	a FALSE value may be erroneously returned.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    TRUE if known to be on last row, FALSE otherwise.
**
** History:
**	 6-Nov-00 (gordy)
**	    Created.
**	14-Sep-05 (gordy)
**	    Return position status when known.
*/

public boolean
isLast()
    throws SQLException
{
    boolean val = (posStatus == ROW  &&  (currentRow.status & Row.LAST) != 0);
    if ( trace.enabled() )  trace.log( title + ".isLast(): " + val );
    return( val );
} // isLast


/*
** Name: rowInserted
**
** Description:
**	Returns an indication that the current row has been inserted.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean		True if row inserted, false otherwise.
**
** History:
**	 4-Aug-03 (gordy)
**	    Implemented using row status info.
*/

public boolean 
rowInserted() throws SQLException
{ 
    boolean inserted = (posStatus == ROW  &&  
			(currentRow.status & Row.INSERTED) != 0);
    if ( trace.enabled() )  trace.log( title + ".rowInserted(): " + inserted );
    return( inserted ); 
} // rowInserted


/*
** Name: rowUpdated
**
** Description:
**	Returns an indication that the current rows has been updated.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean		TRUE if row updated, false otherwise.
**
** History:
**	 4-Aug-03 (gordy)
**	    Implemented using row status info.
*/

public boolean 
rowUpdated() throws SQLException
{ 
    boolean updated = (posStatus == ROW  &&
			(currentRow.status & Row.UPDATED) != 0);
    if ( trace.enabled() )  trace.log( title + ".rowUpdated(): " + updated );
    return( updated ); 
} // rowUpdated


/*
** Name: rowDeleted
**
** Description:
**	Returns an indication that the current row has been deleted.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean		True if row deleted, false otherwise.
**
** History:
**	 4-Aug-03 (gordy)
**	    Implemented using row status info.
*/

public boolean 
rowDeleted() throws SQLException
{ 
    boolean deleted = (posStatus == ROW  &&
			(currentRow.status & Row.DELETED) != 0);
    if ( trace.enabled() )  trace.log( title + ".rowDeleted(): " + deleted );
    return( deleted ); 
} // rowDeleted


/*
** Cursor update methods (not supported at this level).
*/

public void 
insertRow() throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".insertRow()" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); 
} // insertRow

public void 
updateRow() throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".updateRow()" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); 
} // updateRow

public void 
deleteRow() throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".deleteRow()" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); 
} // deleteRow

public void 
moveToInsertRow() throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".moveToInsertRow()" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); 
} // moveToInsertRow

public void 
moveToCurrentRow() throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".moveToCurrentRow()" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); 
} // moveToCurrentRow

public void
cancelRowUpdates() throws SQLException
{ 
    if ( trace.enabled() )  trace.log(title + ".cancelRowUpdates() " ); 
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); 
} // cancelRowUpdates

public void
updateNull( int index ) throws SQLException
{ 
    if ( trace.enabled() ) trace.log( title + ".updateNull()" ); 
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); 
} // updateNull

public void
updateBoolean( int index, boolean value ) throws SQLException
{ 
    if ( trace.enabled() )  trace.log(title + ".updateBoolean()");
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); 
} // updateBoolean

public void
updateByte( int index, byte value ) throws SQLException
{ 
    if ( trace.enabled() )  trace.log(title + ".updateByte()");
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); 
} // updateByte

public void
updateShort( int index, short value ) throws SQLException
{ 
    if ( trace.enabled() )  trace.log(title + ".updateShort()");
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); 
} // updateShort

public void
updateInt( int index, int value ) throws SQLException
{ 
    if ( trace.enabled() )  trace.log(title + ".updateInt()");
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); 
} // updateInt

public void
updateLong( int index, long value ) throws SQLException
{ 
    if ( trace.enabled() )  trace.log(title + ".updateLong()");
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); 
} // updateLong

public void
updateFloat( int index, float value ) throws SQLException
{ 
    if ( trace.enabled() )  trace.log(title + ".updateFloat()");
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); 
} // updateFloat

public void
updateDouble( int index, double value ) throws SQLException
{ 
    if ( trace.enabled() )  trace.log(title + ".updateDouble()");
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); 
} // updateDouble

public void
updateBigDecimal( int index, BigDecimal value ) throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".updateBigDecimal()" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); 
} // updateBigDecimal

public void
updateString( int index, String value ) throws SQLException
{ 
    if ( trace.enabled() )  trace.log(title + ".updateString()");
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); 
} // updateString

public void
updateBytes( int index, byte value[] ) throws SQLException
{ 
    if ( trace.enabled() )  trace.log(title + ".updateBytes()");
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); 
} // updateBytes

public void
updateDate( int index, Date value ) throws SQLException
{ 
    if ( trace.enabled() )  trace.log(title + ".updateDate()");
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); 
} // updateDate

public void
updateTime( int index, Time value ) throws SQLException
{ 
    if ( trace.enabled() )  trace.log(title + ".updateTime()");
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); 
} // updateTime

public void
updateTimestamp( int index, Timestamp value ) throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".updateTimestamp()" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); 
} // updateTimestamp

public void
updateBinaryStream( int index, InputStream in, int length ) 
    throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".updateBinaryStream()" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); 
} // updateBinaryStream

public void
updateAsciiStream( int index, InputStream in, int length ) 
    throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".updateAsciiStream()");
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); 
} // updateAsciiStream

public void
updateCharacterStream( int index, Reader in, int length ) 
    throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".updateCharacterStream()" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); 
} // updateCharacterStream

public void 
updateObject( int index, Object value, int scale ) 
    throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".updateObject()" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); 
} // updateObject

public void 
updateObject( int index, Object value ) throws SQLException
{  
    if ( trace.enabled() )  trace.log( title + ".updateObject()" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); 
} // updateObject

public void
updateArray( int index, Array value ) throws SQLException
{ 
    if (trace.enabled()) trace.log(title + ".updateArray()"); 
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // updateArray

public void
updateBlob( int index, Blob value ) throws SQLException
{ 
    if (trace.enabled()) trace.log(title + ".updateBlob()"); 
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // updateBlob

public void
updateClob( int index, Clob value ) throws SQLException
{ 
    if (trace.enabled()) trace.log(title + ".updateClob()"); 
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // updateClob

public void
updateRef( int index, Ref value ) throws SQLException
{ 
    if (trace.enabled()) trace.log(title + ".updateRef()"); 
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // updateRef

public void 
updateSQLXML(String columnLabel, SQLXML xmlObject)
throws SQLException
{
    if (trace.enabled()) trace.log(title + ".updateSQLXML()"); 
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} //updateSQLXML

public void 
updateSQLXML(int columnIndex, SQLXML xmlObject)
throws SQLException
{
    if (trace.enabled()) trace.log(title + ".updateSQLXML()"); 
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // updateSQLXML

public void 
updateRowId(String columnLabel, RowId x)
throws SQLException
{
    if (trace.enabled()) trace.log(title + ".updateRowId()"); 
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // updateRowId

public void updateRowId(int columnIndex, RowId x)
throws SQLException
{
    if (trace.enabled()) trace.log(title + ".updateRowId()"); 
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} //updateRowId

public void 
updateNClob(int columnIndex, NClob nClob)
throws SQLException
{
    if (trace.enabled()) trace.log(title + ".updateNClob()"); 
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} //updateNClob

public void 
updateNString(int columnIndex, String nString)
throws SQLException
{
    if (trace.enabled()) trace.log(title + ".updateNString()"); 
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} //updateNString

public void 
updateClob(int columnIndex, Reader reader)
throws SQLException
{
    if (trace.enabled()) trace.log(title + ".updateClob()"); 
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} //updateClob

public void 
updateNClob(int columnIndex, Reader reader)
throws SQLException
{
    if (trace.enabled()) trace.log(title + ".updateNClob()"); 
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} //updateNClob

public void 
updateBlob(int columnIndex, InputStream inputStream)
throws SQLException
{
    if (trace.enabled()) trace.log(title + ".updateBlob()"); 
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} //updateBlob

public void 
updateCharacterStream(int columnIndex, Reader x)
throws SQLException
{
    if (trace.enabled()) trace.log(title + ".updateCharacterStream()"); 
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} //updateCharacterStream

public void 
updateBinaryStream(int columnIndex, InputStream x)
throws SQLException
{
    if (trace.enabled()) trace.log(title + ".updateCharacterStream()"); 
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} //updateCharacterStream

public void 
updateAsciiStream(int columnIndex, InputStream x)
throws SQLException
{
    if (trace.enabled()) trace.log(title + ".updateAsciiStream()"); 
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} //updateAsciiStream

public void 
updateNCharacterStream(int columnIndex, Reader x)
throws SQLException
{
    if (trace.enabled()) trace.log(title + ".updateNCharacterStream()"); 
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} //updateNCharacterStream

public void 
updateNClob(int columnIndex, Reader reader, long length)
throws SQLException
{
    if (trace.enabled()) trace.log(title + ".updateNClob()"); 
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} //updateNClob

public void 
updateClob(int columnIndex, Reader reader, long length)
throws SQLException
{
    if (trace.enabled()) trace.log(title + ".updateClob()"); 
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} //updateClob

public void 
updateBlob(int columnIndex, InputStream inputStream, long length)
throws SQLException
{
    if (trace.enabled()) trace.log(title + ".updateBlob()"); 
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} //updateBlob

public void 
updateCharacterStream(int columnIndex, Reader x, long length)
throws SQLException
{
    if (trace.enabled()) trace.log(title + ".updateCharacterStream()"); 
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} //updateCharacterStream

public void 
updateBinaryStream(int columnIndex, InputStream x, long length)
throws SQLException
{
    if (trace.enabled()) trace.log(title + ".updateBinaryStream()"); 
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} //updateBinaryStream

public void 
updateAsciiStream(int columnIndex, InputStream x, long length)
throws SQLException
{
    if (trace.enabled()) trace.log(title + ".updateAsciiStream()"); 
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} //updateAsciiStream

public void 
updateNCharacterStream(int columnIndex, Reader x, long length)
throws SQLException
{
    if (trace.enabled()) trace.log(title + ".updateNCharacterStream()"); 
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} //updateNCharacterStream




/*
** Data update methods which are simple covers for
** the primary data update methods.
*/

public void 
updateNull( String name ) throws SQLException
{ updateNull( columnByName( name ) ); }

public void 
updateBoolean( String name, boolean value ) throws SQLException
{ updateBoolean( columnByName( name ), value ); }

public void 
updateByte( String name, byte value ) throws SQLException
{ updateByte( columnByName( name ), value ); }

public void 
updateShort( String name, short value ) throws SQLException
{ updateShort( columnByName( name ), value ); }

public void 
updateInt( String name, int value ) throws SQLException
{ updateInt( columnByName( name ), value ); }

public void 
updateLong( String name, long value ) throws SQLException
{ updateLong( columnByName( name ), value ); }

public void 
updateFloat( String name, float value ) throws SQLException
{ updateFloat( columnByName( name ), value ); }

public void 
updateDouble( String name, double value ) throws SQLException
{ updateDouble( columnByName( name ), value ); }

public void 
updateBigDecimal( String name, BigDecimal value ) throws SQLException
{ updateBigDecimal( columnByName( name ), value ); }

public void 
updateString( String name, String value ) throws SQLException
{ updateString( columnByName( name ), value ); }

public void 
updateBytes( String name, byte value[] ) throws SQLException
{ updateBytes( columnByName( name ), value ); }

public void 
updateDate( String name, Date value ) throws SQLException
{ updateDate( columnByName( name ), value ); }

public void 
updateTime( String name, Time value ) throws SQLException
{ updateTime( columnByName( name ), value ); }

public void 
updateTimestamp( String name, Timestamp value ) throws SQLException
{ updateTimestamp( columnByName( name ), value ); }

public void 
updateBinaryStream( String name, InputStream in, int length ) 
    throws SQLException
{ updateBinaryStream( columnByName( name ), in, length ); }

public void 
updateAsciiStream( String name, InputStream in, int length ) 
    throws SQLException
{ updateAsciiStream( columnByName( name ), in, length ); }

public void 
updateCharacterStream( String name, Reader in, int length ) 
    throws SQLException
{ updateCharacterStream( columnByName( name ), in, length ); }

public void 
updateObject( String name, Object value ) throws SQLException
{ updateObject( columnByName( name ), value ); 
} // updateObject

public void 
updateObject( String name, Object value, int scale )  throws SQLException
{ updateObject( columnByName( name ), value, scale ); }

public void
updateArray( String name, Array value ) throws SQLException
{ updateArray( columnByName( name ), value ); }

public void
updateBlob( String name, Blob value ) throws SQLException
{ updateBlob( columnByName( name ), value ); }

public void
updateClob( String name, Clob value ) throws SQLException
{ updateClob( columnByName( name ), value ); }

public void
updateRef( String name, Ref value ) throws SQLException
{ updateRef( columnByName( name ), value ); }

public void 
updateNClob(String name, NClob nclob)
throws SQLException
{ updateClob( columnByName( name ), (Clob) nclob ); }

public void 
updateNString(String name, String nString)
throws SQLException
{ updateNString( columnByName( name ), nString ); }

public void 
updateNClob(String name, Reader reader)
throws SQLException
{ updateNClob( columnByName( name ), reader ); }

public void 
updateClob(String name, Reader reader)
throws SQLException
{ updateClob( columnByName( name ), reader ); }

public void 
updateBlob(String name, InputStream stream)
throws SQLException
{ updateBlob( columnByName( name ), stream ); } 

public void 
updateCharacterStream(String name, Reader reader)
throws SQLException
{ updateCharacterStream(  columnByName( name ), reader ); }

public void 
updateBinaryStream(String name, InputStream stream)
throws SQLException
{ updateBinaryStream( columnByName(name), stream ); }

public void 
updateAsciiStream(String name, InputStream stream)
throws SQLException
{ updateAsciiStream( columnByName(name), stream ); }

public void 
updateNCharacterStream(String name, Reader reader)
throws SQLException
{ updateNCharacterStream( columnByName(name), reader ); }

public void 
updateNClob(String name, Reader reader, long length)
throws SQLException
{ updateNClob( columnByName(name), reader, length); }

public void 
updateClob(String name, Reader reader, long length)
throws SQLException
{ updateClob( columnByName( name ), reader, length); }

public void 
updateBlob(String name, InputStream inputStream, long length)
throws SQLException
{ updateBlob( columnByName( name ), inputStream, length); }

public void 
updateCharacterStream(String name, Reader reader, long length)
throws SQLException
{ updateCharacterStream(  columnByName( name ), reader, length ); }

public void 
updateBinaryStream(String name, InputStream stream, long length)
throws SQLException
{ updateBinaryStream( columnByName(name), stream, length ); }

public void 
updateAsciiStream(String name, InputStream stream, long length)
throws SQLException
{ updateAsciiStream( columnByName(name), stream, length ); }

public void 
updateNCharacterStream(String name, Reader reader, long length)
throws SQLException
{ updateNCharacterStream( columnByName(name), reader, length ); }

/*
** Name: findColumn
**
** Description:
**	Determine column index from column name.
**
** Input:
**	name	Name of the column.
**
** Output:
**	None.
**
** Returns:
**	int	Column index, 0 if not found.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	04-Feb-00 (rajus01)
**	    Throw exception for invalid column name.
**	22-sep-03 (gordy)
**	    Extracted search functionality to getColByName().
*/

public int
findColumn( String name )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".findColumn('" + name + "')" );
    int col;
    
    try { col = columnByName( name ); }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  trace.log( title + ".findColumn: not found" );
	throw ex;
    }

    if ( trace.enabled() )  trace.log( title + ".findColumn: " + col );
    return( col );
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
    if ( trace.enabled() )  trace.log( title + ".wasNull(): " + isNull );
    return( isNull );
} // wasNull


/*
** Name: getBoolean
**
** Description:
**	Retrieve column data as a boolean value.  If the column
**	is NULL, false is returned.
**
** Input:
**	index	    Column index.
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
**	31-Oct-02 (gordy)
**	    Encapsulated common BLOB column processing in methods.
**	20-Mar-03 (gordy)
**	    JDBC added BOOLEAN data type, treat same as BIT.
**	26-Sep-03 (gordy)
**	    Raw data values replaced with SqlData objects.
*/

public boolean
getBoolean( int index )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getBoolean( " + index + " )" );
    
    SqlData	data = columnDataValue( index );
    boolean	value = data.isNull() ? false : data.getBoolean();
    
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
**	index	    Column index.
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
**	31-Oct-02 (gordy)
**	    Encapsulated common BLOB column processing in methods.
**	20-Mar-03 (gordy)
**	    JDBC added BOOLEAN data type, treat same as BIT.
**	26-Sep-03 (gordy)
**	    Raw data values replaced with SqlData objects.
*/

public byte
getByte( int index )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getByte( " + index + " )" );
    
    SqlData	data = columnDataValue( index );
    byte 	value = data.isNull() ? 0 : data.getByte();

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
**	index	    Column index.
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
**	31-Oct-02 (gordy)
**	    Encapsulated common BLOB column processing in methods.
**	20-Mar-03 (gordy)
**	    JDBC added BOOLEAN data type, treat same as BIT.
**	26-Sep-03 (gordy)
**	    Raw data values replaced with SqlData objects.
*/

public short
getShort( int index )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getShort( " + index + " )" );
    
    SqlData	data = columnDataValue( index );
    short 	value = data.isNull() ? 0 : data.getShort();

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
**	index	    Column index.
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
**	31-Oct-02 (gordy)
**	    Encapsulated common BLOB column processing in methods.
**	20-Mar-03 (gordy)
**	    JDBC added BOOLEAN data type, treat same as BIT.
**	26-Sep-03 (gordy)
**	    Raw data values replaced with SqlData objects.
*/

public int
getInt( int index )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getInt( " + index + " )" );
    
    SqlData	data = columnDataValue( index );
    int 	value = data.isNull() ? 0 : data.getInt();

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
**	index	    Column index.
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
**	31-Oct-02 (gordy)
**	    Encapsulated common BLOB column processing in methods.
**	20-Mar-03 (gordy)
**	    JDBC added BOOLEAN data type, treat same as BIT.
**	26-Sep-03 (gordy)
**	    Raw data values replaced with SqlData objects.
*/

public long
getLong( int index )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getLong( " + index + " )" );

    SqlData	data = columnDataValue( index );
    long	value = data.isNull() ? 0L : data.getLong();

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
**	index	    Column index.
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
**	31-Oct-02 (gordy)
**	    Encapsulated common BLOB column processing in methods.
**	20-Mar-03 (gordy)
**	    JDBC added BOOLEAN data type, treat same as BIT.
**	26-Sep-03 (gordy)
**	    Raw data values replaced with SqlData objects.
*/

public float
getFloat( int index )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getFloat( " + index + " )" );

    SqlData	data = columnDataValue( index );
    float	value = data.isNull() ? 0.0F : data.getFloat();

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
**	index	    Column index.
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
**	31-Oct-02 (gordy)
**	    Encapsulated common BLOB column processing in methods.
**	20-Mar-03 (gordy)
**	    JDBC added BOOLEAN data type, treat same as BIT.
**	26-Sep-03 (gordy)
**	    Raw data values replaced with SqlData objects.
*/

public double
getDouble( int index )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getDouble( " + index + " )" );
    
    SqlData	data = columnDataValue( index );
    double	value = data.isNull() ? 0.0 : data.getDouble();

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
**	index	    Column index.
**
** Output:
**	None.
**
** Returns:
**	BigDecimal  Column numeric value.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public BigDecimal
getBigDecimal( int index )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getBigDecimal( " + index + " )" );

    SqlData	data = columnDataValue( index );
    BigDecimal	value = data.isNull() ? null : data.getBigDecimal();    

    if ( trace.enabled() )  trace.log( title + ".getBigDecimal: " + value );
    return( value );
} // getBigDecimal


/*
** Name: getBigDecimal
**
** Description:
**	Retrieve column data as a BigDecimal value.  If the column
**	is NULL, null is returned.
**
** Input:
**	index	    Column index.
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
**	31-Oct-02 (gordy)
**	    Encapsulated common BLOB column processing in methods.
**	20-Mar-03 (gordy)
**	    JDBC added BOOLEAN data type, treat same as BIT.
**	26-Sep-03 (gordy)
**	    Raw data values replaced with SqlData objects.
**	 1-Dec-03 (gordy)
**	    Only support positive scale values, extracted non-scale version.
*/

public BigDecimal
getBigDecimal( int index, int scale )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getBigDecimal( " + 
				       index + ", " + scale + " )" );

    SqlData	data = columnDataValue( index );
    BigDecimal	value = data.isNull() ? null : data.getBigDecimal( scale );    

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
**	index	    Column index.
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
**	31-Oct-02 (gordy)
**	    Encapsulated common BLOB column processing in methods.
**	20-Mar-03 (gordy)
**	    JDBC added BOOLEAN data type, treat same as BIT.
**	15-Apr-03 (gordy)
**	    Abstracted date formatters.
**	 7-Jul-03 (gordy)
**	    Since we return 0 length string for empty dates, don't
**	    return time component for date only values.  Generate
**	    data truncation warning for this case.
**	26-Sep-03 (gordy)
**	    Raw data values replaced with SqlData objects.
*/

public String
getString( int index )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getString( " + index + " )" );

    SqlData	data = columnDataValue( index );
    String	value;

    if ( data.isNull() )
	value = null;
    else  if ( rs_max_len > 0 )
	value = data.getString( rs_max_len );
    else
	value = data.getString();
    
    /*
    ** Ingres dates include intervals which are returned
    ** as raw strings by IngresDate.getString().  We want
    ** to set a warning for intervals, so a data specific
    ** check is required.
    */
    if ( data instanceof IngresDate  &&  ((IngresDate)data).isInterval() )
    {
        /*
        ** A truncation warning was most likely generated
        ** because of the interval, so drop that warning
        ** prior to generating the interval warning.
        */
        warnings = null;
        setWarning( SqlWarn.get( ERR_GC401B_INVALID_DATE ) );
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
**	index	    Column index.
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
**	31-Oct-02 (gordy)
**	    Encapsulated common BLOB column processing in methods.
**	26-Sep-03 (gordy)
**	    Raw data values replaced with SqlData objects.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Clean up tracing of array parameter.
*/

public byte[]
getBytes( int index )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getBytes( " + index + " )" );
    
    SqlData	data = columnDataValue( index );
    byte	value[];
			    
    if ( data.isNull() )
	value = null;
    else  if ( rs_max_len > 0 )
	value = data.getBytes( rs_max_len );
    else
	value = data.getBytes();
    
    if ( trace.enabled() )  
	trace.log( title + ".getBytes: " + 
		   (value == null ? "null" : "[" + value.length + "]") );
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
**	index	    Column index.
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
**	31-Oct-02 (gordy)
**	    Encapsulated common BLOB column processing in methods.
**	15-Apr-03 (gordy)
**	    Abstracted date formatters.
**	21-Jul-03 (gordy)
**	    Apply calendar generically after processing connection
**	    relative processing.
**	26-Sep-03 (gordy)
**	    Raw data values replaced with SqlData objects.
**	17-Feb-09 (gordy)
**	    Don't generate truncation warnings for IngresDate values
**	    which are date-only values.
*/

public Date
getDate( int index, Calendar cal )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getDate( " + index + 
			    ((cal == null) ? " )" : ", " + cal + " )") );
    
    TimeZone	tz = (cal == null) ? null : cal.getTimeZone();
    SqlData	data = columnDataValue( index );
    Date	value = data.isNull() ? null : data.getDate( tz ); 

    /*
    ** IngresDate values are timestamps overloaded with
    ** date-only and empty-date values.  Data truncation 
    ** is indicated for anything other than a timestamp 
    ** value.  If only a date is being retrieved, then
    ** data truncation should only be indicated for 
    ** empty-date values.
    */
    if ( data instanceof IngresDate  &&
         data.isTruncated()  &&  data.getTruncSize() > 0 )
        warnings = null;

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
**	index	    Column index.
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
**	31-Oct-02 (gordy)
**	    Encapsulated common BLOB column processing in methods.
**	15-Apr-03 (gordy)
**	    Abstracted date formatters.
**	 7-Jul-04 (gordy)
**	    Generate data truncation warning for date only values.
**	21-Jul-03 (gordy)
**	    Apply calendar generically after processing connection
**	    relative processing.
**	26-Sep-03 (gordy)
**	    Raw data values replaced with SqlData objects.
*/

public Time
getTime( int index, Calendar cal )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getTime( " + index + 
			    ((cal == null) ? " )" : ", " + cal + " )") );
    
    TimeZone	tz = (cal == null) ? null : cal.getTimeZone();
    SqlData	data = columnDataValue( index );
    Time	value = data.isNull() ? null : data.getTime( tz );

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
**	index	    Column index.
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
**	31-Oct-02 (gordy)
**	    Encapsulated common BLOB column processing in methods.
**	15-Apr-03 (gordy)
**	    Abstracted date formatters.
**	 7-Jul-03 (gordy)
**	    Generate data truncation warning for date only values.
**	21-Jul-03 (gordy)
**	    Apply calendar generically after processing connection
**	    relative processing.
**	26-Sep-03 (gordy)
**	    Raw data values replaced with SqlData objects.
*/

public Timestamp
getTimestamp( int index, Calendar cal )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getTimestamp( " + index + 
			    ((cal == null) ? " )" : ", " + cal + " )") );
    
    TimeZone	tz = (cal == null) ? null : cal.getTimeZone();
    SqlData	data = columnDataValue( index );
    Timestamp	value = data.isNull() ? null : data.getTimestamp( tz );

    if ( trace.enabled() )  trace.log( title + ".getTimestamp: " + value );
    return( value );
} // getTimestamp


/*
** Name: getBinaryStream
**
** Description:
**	Retrieve column data as a binary stream.  If the column
**	is NULL, null is returned.  If the column can be converted 
**	to a binary stream, a binary input stream is returned.  
**	Otherwise, a conversion exception is thrown.
**
** Input:
**	index		Column index.
**
** Output:
**	None.
**
** Returns:
**	InputStream	Binary input stream.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented BLOB data support.
**      19-Jul-02 (gordy)
**          Marked blob stream used after consuming contents so that sub-
**          classes which gain control during exception conditions will
**          be able to detect that the blob needs processing.
**	31-Oct-02 (gordy)
**	    Separated column value processing to helper method.
**	26-Sep-03 (gordy)
**	    Raw data values replaced with SqlData objects.
**	14-Sep-05 (gordy)
**	    Trace exceptions.
*/

public InputStream
getBinaryStream( int index )
    throws SQLException
{
    SqlData	data;
    InputStream value;

    if ( trace.enabled() )  
	trace.log( title + ".getBinaryStream( " + index + " )" );
    
    data = columnDataValue( index );

    try { value = data.isNull() ? null : data.getBinaryStream(); }
    catch( SQLException ex )
    {
	if ( trace.enabled() )
	    trace.log( title + ".getBinaryStream: error accessing BLOB data" );
	if ( trace.enabled( 1 ) )  SqlExFactory.trace( ex, trace );
	throw ex;
    }

    if ( trace.enabled() )  trace.log( title + ".getBinaryStream: " + value );
    return( value );
} // getBinaryStream


/*
** Name: getAsciiStream
**
** Description:
**	Retrieve column data as an ASCII character stream.  If the column
**	is NULL, null is returned.  If the column can be converted to an 
**	ASCII stream, an ASCII input stream is returned.  Otherwise, a 
**	conversion exception is thrown.
**
** Input:
**	index	Column index.
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
**	04-Feb-99 (rajus01)
**	    Use bin2str() to convert BLOB to String.	
**	31-Oct-02 (gordy)
**	    Encapsulated common BLOB column processing in methods.
**	26-Sep-03 (gordy)
**	    Raw data values replaced with SqlData objects.
**	14-Sep-05 (gordy)
**	    Trace exceptions.
*/

public InputStream
getAsciiStream( int index )
    throws SQLException
{
    SqlData	data;
    InputStream value;

    if ( trace.enabled() )  
	trace.log( title + ".getAsciiStream( " + index + " )" );

    data = columnDataValue( index );

    try { value = data.isNull() ? null : data.getAsciiStream(); }
    catch( SQLException ex )
    {
	if ( trace.enabled() )
	    trace.log( title + ".getAsciiStream: error accessing BLOB data" );
	if ( trace.enabled( 1 ) )  SqlExFactory.trace( ex, trace );
	throw ex;
    }

    if ( trace.enabled() )  trace.log( title + ".getAsciiStream: " + value );
    return( value );
} // getAsciiStream


/*
** Name: getUnicodeStream
**
** Description:
**	Retrieve column data as a Unicode stream.  If the column
**	is NULL, null is returned.  If the column can be converted 
**	to a Unicode stream, a Unicode input stream is returned.  
**	Otherwise, a conversion exception is thrown.
**
** Input:
**	index		Column index.
**
** Output:
**	None.
**
** Returns:
**	InputStream	Unicode character input stream.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented BLOB data support.
**	04-Feb-99 (rajus01)
**	    Use bin2str() to convert BLOB to String.
**	13-Jun-01 (gordy)
**	    Replaced UniStrm class with method which performs converts
**	    UCS2 characters to UTF-8 byte format.	
**	31-Oct-02 (gordy)
**	    Encapsulated common BLOB column processing in methods.
**	26-Sep-03 (gordy)
**	    Raw data values replaced with SqlData objects.
**	14-Sep-05 (gordy)
**	    Trace exceptions.
*/

public InputStream
getUnicodeStream( int index )
    throws SQLException
{
    SqlData	data;
    InputStream value;

    if ( trace.enabled() )  
	trace.log( title + ".getUnicodeStream( " + index + " )" );
    
    data = columnDataValue( index );

    try { value = data.isNull() ? null : data.getUnicodeStream(); }
    catch( SQLException ex )
    {
	if ( trace.enabled() )
	    trace.log( title + ".getUnicodeStream: error accessing BLOB data" );
	if ( trace.enabled( 1 ) )  SqlExFactory.trace( ex, trace );
	throw ex;
    }

    if ( trace.enabled() )  trace.log( title + ".getUnicodeStream: " + value );
    return( value );
} // getUnicodeStream


/*
** Name: getCharacterStream
**
** Description:
**	Retrieve column data as a character stream.  If the column
**	is NULL, null is returned.  If the column can be converted 
**	to a character stream, a character Reader is returned.  
**	Otherwise, a conversion exception is thrown.
**
** Input:
**	index	Column index.
**
** Output:
**	None.
**
** Returns:
**	Reader	Character stream.
**
** History:
**	 6-Nov-00 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Encapsulated common BLOB column processing in methods.
**	26-Sep-03 (gordy)
**	    Raw data values replaced with SqlData objects.
**	14-Sep-05 (gordy)
**	    Trace exceptions.
*/

public Reader
getCharacterStream( int index )
    throws SQLException
{
    SqlData	data;
    Reader	value;

    if ( trace.enabled() )  
	trace.log( title + ".getCharacterStream( " + index + " )" );

    data = columnDataValue( index );

    try { value = data.isNull() ? null : data.getCharacterStream(); }
    catch( SQLException ex )
    {
	if ( trace.enabled() )
	    trace.log(title + ".getCharacterStream: error accessing BLOB data");
	if ( trace.enabled( 1 ) )  SqlExFactory.trace( ex, trace );
	throw ex;
    }

    if ( trace.enabled() )  trace.log(title + ".getCharacterStream: " + value);
    return( value );
} // getCharacterStream


/*
** Name: getBlob
**
** Description:
**	Retrieve column data as a Blob object.  If the column is
**	NULL, null is returned.
**
** Input:
**	index	Column index.
**
** Output:
**	None.
**
** Returns:
**	Blob	Binary Large Object
**
** History:
**	15-Nov-06 (gordy)
**	    Implemented.
*/

public Blob
getBlob( int index )  
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getBlob( " + index + " )" );
    
    SqlData	data = columnDataValue( index );
    Blob	value;
			    
    if ( data.isNull() )
	value = null;
    else
	value = data.getBlob();
    
    if ( trace.enabled() )  trace.log( title + ".getBlob: " + value );
    return( value );
} // getBlob


/*
** Name: getClob
**
** Description:
**	Retrieve column data as a Clob object.  If the column is
**	NULL, null is returned.
**
** Input:
**	index	Column index.
**
** Output:
**	None.
**
** Returns:
**	Clob	Character Large Object
**
** History:
**	15-Nov-06 (gordy)
**	    Implemented.
*/

public Clob
getClob( int index )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getClob( " + index + " )" );
    
    SqlData	data = columnDataValue( index );
    Clob	value;
			    
    if ( data.isNull() )
	value = null;
    else
	value = data.getClob();
    
    if ( trace.enabled() )  trace.log( title + ".getClob: " + value );
    return( value );
} // getClob

/*
** Name: getNClob
**
** Description:
**	Retrieve column data as a NClob object.  If the column is
**	NULL, null is returned.
**
** Input:
**	index	Column index.
**
** Output:
**	None.
**
** Returns:
**	NClob	National Character Large Object
**
** History:
**	9-Apr-09 (rajus01)
**	    Implemented.
*/

public NClob
getNClob( int index )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getNClob( " + index + " )" );
    
    SqlData	data = columnDataValue( index );
    NClob	value;
			    
    if ( data.isNull() )
	value = null;
    else
	value = data.getNlob();
    
    if ( trace.enabled() )  trace.log( title + ".getNClob: " + value );
    return( value );
} // getNClob

/*
** Name: getObject
**
** Description:
**	Retrieve column data as a generic Java object.  If the column
**	is NULL, null is returned.
**
** Input:
**	index	Column index.
**
** Output:
**	None.
**
** Returns:
**	Object	Java object.
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
**	31-Oct-02 (gordy)
**	    Encapsulated common BLOB column processing in methods.
**	15-Apr-03 (gordy)
**	    Abstracted date formatters.
**	26-Sep-03 (gordy)
**	    Raw data values replaced with SqlData objects.
*/

public Object
getObject( int index )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getObject( " + index + " )" );
    
    SqlData	data = columnDataValue( index );
    Object	value;
    
    if ( data.isNull() )
	value = null;
    else  if ( rs_max_len > 0 )
	value = data.getObject( rs_max_len );
    else
	value = data.getObject();
	
    if ( trace.enabled() )  trace.log(title + ".getObject: " + value);
    return( value );
} // getObject


/*
** Data retrieve methods which are for unsupported types
*/

public Array
getArray( int index )  throws SQLException
{
    if (trace.enabled())  trace.log(title + ".getArray( " + index + " )");
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // getArray

public Ref
getRef( int index )  throws SQLException
{
    if (trace.enabled())  trace.log(title + ".getRef( " + index + " )");
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // getRef

public URL
getURL( int index )  throws SQLException
{
    if (trace.enabled())  trace.log(title + ".getURL( " + index + " )");
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // getURL

public SQLXML 
getSQLXML(String colLabel)
throws SQLException
{
    if (trace.enabled())  trace.log(title + ".getSQLXML( " + colLabel + " )");
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
}

public SQLXML 
getSQLXML(int columnIdx)
throws SQLException
{
    if (trace.enabled())  trace.log(title + ".getSQLXML( " + columnIdx + " )");
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
}

public RowId 
getRowId(String columnLabel)
throws SQLException
{
    if (trace.enabled())  trace.log(title + ".getRowId( " + columnLabel + " )");
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // getRowId

public RowId 
getRowId(int columnIndex)
throws SQLException
{
    if (trace.enabled())  trace.log(title + ".getRowId( " + columnIndex + " )");
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // getRowId

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
   if ( trace.enabled() )
       trace.log(title + ".isWrapperFor(" + iface + ")");
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
} //unwrap

/*
** Data access methods which are simple covers for
** the primary data access method.
*/

public boolean 
getBoolean( String name ) throws SQLException
{ return( getBoolean( columnByName( name ) ) ); } 

public byte 
getByte( String name ) throws SQLException
{ return( getByte( columnByName( name ) ) ); } 

public short 
getShort( String name ) throws SQLException
{ return( getShort( columnByName( name ) ) ); }

public int 
getInt( String name ) throws SQLException
{ return( getInt( columnByName( name ) ) ); }

public long 
getLong( String name ) throws SQLException
{ return( getLong( columnByName( name ) ) ); }

public float 
getFloat( String name ) throws SQLException
{ return( getFloat( columnByName( name ) ) ); }

public double 
getDouble( String name ) throws SQLException
{ return( getDouble( columnByName( name ) ) ); }

public BigDecimal 
getBigDecimal( String name ) throws SQLException
{ return( getBigDecimal( columnByName( name ) ) ); }

public BigDecimal 
getBigDecimal( String name, int scale ) throws SQLException
{ return( getBigDecimal( columnByName( name ), scale ) ); }

public String 
getString( String name ) throws SQLException
{ return( getString( columnByName( name ) ) ); }

public byte[] 
getBytes( String name ) throws SQLException
{ return( getBytes( columnByName( name ) ) ); }

public Date 
getDate( int index ) throws SQLException
{ return( getDate( index, null ) ); }

public Date 
getDate( String name ) throws SQLException
{ return( getDate( columnByName( name ), null ) ); }

public Date 
getDate( String name, Calendar cal ) throws SQLException
{ return( getDate( columnByName( name ), cal ) ); }

public Time 
getTime( int index ) throws SQLException
{ return( getTime( index, null ) ); }

public Time 
getTime( String name ) throws SQLException
{ return( getTime( columnByName( name ), null ) ); }

public Time 
getTime( String name, Calendar cal ) throws SQLException
{ return( getTime( columnByName( name ), cal ) ); }

public Timestamp 
getTimestamp( int index ) throws SQLException
{ return( getTimestamp( index, null ) ); }

public Timestamp 
getTimestamp( String name ) throws SQLException
{ return( getTimestamp( columnByName( name ), null ) ); }

public Timestamp 
getTimestamp( String name, Calendar cal ) throws SQLException
{ return( getTimestamp( columnByName( name ), cal ) ); }

public InputStream 
getBinaryStream( String name ) throws SQLException
{ return( getBinaryStream( columnByName( name ) ) ); }

public InputStream 
getAsciiStream( String name ) throws SQLException
{ return( getAsciiStream( columnByName( name ) ) ); }

public InputStream 
getUnicodeStream( String name ) throws SQLException
{ return( getUnicodeStream( columnByName( name ) ) ); }

public Reader 
getCharacterStream( String name ) throws SQLException
{ return( getCharacterStream( columnByName( name ) ) ); }

public Object 
getObject( String name ) throws SQLException
{ return( getObject( columnByName( name ) ) ); }

public Object 
getObject( int index, Map map ) throws SQLException
{ return( getObject( index ) ); }

public Object 
getObject( String name, Map map ) throws SQLException
{ return( getObject( columnByName( name ) ) ); }

public Array 
getArray( String name ) throws SQLException
{ return( getArray( columnByName( name ) ) ); }

public Blob 
getBlob( String name ) throws SQLException
{ return( getBlob( columnByName( name ) ) ); }

public Clob 
getClob( String name ) throws SQLException
{ return( getClob( columnByName( name ) ) ); }

public Ref 
getRef( String name ) throws SQLException
{ return( getRef( columnByName( name ) ) ); } 

public URL 
getURL( String name ) throws SQLException
{ return( getURL( columnByName( name ) ) ); } 

public Reader 
getNCharacterStream(String name) throws SQLException
{ return( getCharacterStream( columnByName( name ) ) ); }

public Reader 
getNCharacterStream(int columnIndex) throws SQLException
{ return( getCharacterStream( columnIndex ) ); }

public String 
getNString(String name) throws SQLException
{ return( getString( columnByName( name ) ) ); }

public 
String getNString(int columnIndex) throws SQLException
{ return(getString( columnIndex )); }

public NClob getNClob(String columnLabel) throws SQLException
{ return( getNClob( columnByName( columnLabel ) ) ); }

/*
** Name: columnMap
**
** Description:
**	Determines if a given column index is a part of the result
**	set and maps the external index to the internal index.
**
** Input:
**	index	External column index [1,rsmd.count].
**
** Output:
**	None.
**
** Returns:
**	int		Internal column index [0,rsmd.count - 1].
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	 4-Apr-01 (gordy)
**	    Clear warnings as common functionality.
**	18-May-01 (gordy)
**	    Throw more meaningful exception when closed.
**	 4-Aug-03 (gordy)
**	    Replaced row cache with status info.  Partial row
**	    fetches must be handled by sub-classes.
**	 1-Dec-03 (gordy)
**	    Separate invalid row exceptions from index range.
**	20-Jul-07 (gordy)
**	    Ensure column data is loaded and available.
*/

protected int
columnMap( int index )
    throws SQLException
{
    if ( posStatus == CLOSED ) 
	throw SqlExFactory.get( ERR_GC401D_RESULTSET_CLOSED );

    if ( posStatus != ROW  ||  (currentRow.status & Row.DELETED) != 0 )
	throw SqlExFactory.get( ERR_GC4021_INVALID_ROW ); 
    
    if ( index < 1  ||  index > rsmd.count )
	throw SqlExFactory.get( ERR_GC4011_INDEX_RANGE );

    index--;	// Internally, column indexes start at 0.

    /*
    ** Check that column is available, request loading if not.
    */
    if ( index >= currentRow.count )  flush( index );
    return( index );
} // columnMap


/*
** Name: getSqlData
**
** Description:
**	Returns the SqlData object for a column.
**
** Input:
**	index	Internal column index [0,rsmd.count - 1].
**
** Output:
**	None.
**
** Returns:
**	SqlData	Column data value.
**
** History:
**	26-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Moved functionality to columnDataValue().  Now simply
**	    returns column SqlData object from column data array.
**	    Provides ability for sub-classes to override when data
**	    stored externally.
*/

protected SqlData
getSqlData( int index )
    throws SQLException
{
    return( currentRow.columns[ index ] );
} // getSqlData


/*
** Name: columnByName
**
** Description:
**	Search for column with matching name and return column index.
**
** Input:
**	name	Column name.
**
** Output:
**	None.
**
** Returns:
**	int	Column index.
**
** History:
**	22-Sep-03 (gordy)
**	    Extracted from findColumn().
**	 1-Dec-03 (gordy)
**	    Renamed and made private.
*/

private int
columnByName( String name )
    throws SQLException
{
    for( int col = 0; col < rsmd.count; col++ )
	if ( rsmd.desc[ col ].name != null  &&
	     rsmd.desc[ col ].name.equalsIgnoreCase( name ) )
	{
	    col++;
	    if ( trace.enabled( 5 ) )  
		trace.write( tr_id + ": column '" + name + "' => " + col );
	    return( col );
	}

    if ( trace.enabled( 5 ) )  
	trace.write( tr_id + ": column '" + name + "' not found!" );
    throw SqlExFactory.get( ERR_GC4012_INVALID_COLUMN_NAME );
} // columnByName


/*
** Name: columnDataValue
**
** Description:
**	Performs generic column data access operations:
**	sets isNull indicator and checks data truncation.  
**	Returns the SqlData object for the column.
**
** Input:
**	index		External column index [1,rsmd.count].
**
** Output:
**	None.
**
** Returns:
**	SqlData		Column data object.
**
** History:
**	 1-Dec-03 (gordy)
**	    Extracted from getSqlData() which is now used to
**	    simply obtain the SqlData object.
*/

private synchronized SqlData
columnDataValue( int index )
    throws SQLException
{
    warnings = null;

    SqlData data = getSqlData( columnMap( index ) );
    isNull = data.isNull();

    if ( ! isNull  &&  data.isTruncated() )  
	setWarning( new DataTruncation( index, false, true, 
					data.getDataSize(), 
					data.getTruncSize() ) );
    return( data );
} // columnDataValue


/*
** Name: Row
**
** Description:
**
**
**  Constants:
**
**	FIRST		Row is first row.
**	LAST		Row is last row.
**	INSERTED	Row has been inserted.
**	UPDATED		Row has been updated.
**	DELETED		Row has been deleted.
**
**  Public Data:
**
**	status		Status of row.
**	id		Row number.
**	count		Number of available columns.
**	columns		Array of column data value objects.
**
** History:
**	20-Jul-07 (gordy)
**	    Created.
*/

protected static class
Row
{
    /*
    ** Status flags, multiple valid for a given row.
    */
    public static final int	FIRST   = 0x01;
    public static final int	LAST    = 0x02;
    public static final int	INSERTED= 0x10;
    public static final int	UPDATED	= 0x20;
    public static final int	DELETED	= 0x40;
	     
    public int		status = 0;		// Row status flags.
    public int		id = 0;			// Row number.
    public int		count = 0;		// Number of available columns.
    public SqlData	columns[] = null;	// Column data.

} // class Row


} // class JdbcRslt

