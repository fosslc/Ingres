/*
** Copyright (c) 2002, 2006 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.Data;
using System.Collections;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: advanrslt.cs
	**
	** Description:
	**	Defines an abstract base class which implements the provider's ResultSet.
	**
	**  Classes:
	**
	**	AdvanRslt
	**
	** History:
	**	14-May-99 (gordy)
	**	    Created.
	**	 8-Sep-99 (gordy)
	**	    Created new base class for class which interact with
	**	    the server and extracted common data and methods.
	**	    Synchronize entire request/response with server.
	**	13-Sep-99 (gordy)
	**	    Implemented error code support.
	**	29-Sep-99 (gordy)
	**	    Added class to support BLOBs as streams.  Implemented
	**	    BLOB data support.  Use DbConn lock()/unlock() methods
	**	    for synchronization.
	**	11-Nov-99 (gordy)
	**	    Extracted base functionality from AdvanResult.
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
	**	    Added support for 2.0 extensions.
	**	23-Jan-01 (gordy)
	**	    Changed parameter type to AdvanStmt for backward compatibility.
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
	**	    UCS-2 character arrays into UTF-8 byte format.  2.1 spec
	**	    requires unicode streams to be UTF-8 encoded.
	**	19-Jul-02 (gordy)
	**	    Marked blob stream used after consuming contents so that sub-
	**	    classes which gain control during exception conditions will
	**	    be able to detect that the blob needs processing.
	**	 7-Aug-02 (gordy)
	**	    Return empty string for Ingres empty date.
	**	26-Aug-02 (thoda04)
	**	    Ported for .NET Provider.
	**	31-Oct-02 (gordy)
	**	    Adapted for generic GCF driver.
	**	20-Mar-03 (gordy)
	**	    Upgraded to 3.0.
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
	**	25-Feb-04 (thoda04)
	**	    Convert SqlEx exception to an IngresException.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	 2-jan-05 (thoda04) Bug 113695
	**	    getColByName should return zero based index.
	*/



	/*
	** Name: AdvanRslt
	**
	** Description:
	**	Abstract driver base class which implements the
	**	ResultSet interface.  Provides access to column values for
	**	current row.  
	**
	**	A result set represents a set of N rows which are numbered,
	**	if N > 0, 1 to N.  A result set may be positioned before the
	**	first row and after the last row, so the virtual row numbers
	**	range from 0 to N+1 (0 representing the position prior to the
	**	first row and N+1 the position after the last row).
	**
	**	Sub-classes are responsible for loading data and must implement
	**	the load() method which is called when the next row is to be
	**	made available.  Sub-classes must maintain four fields when
	**	loading rows: pos_status which indicates where the result set
	**	is positioned, row_status which provides information on the
	**	state of the current row, row_number which is the number of
	**	the current row (0 to N+1) and columns which is array of SQL
	**	data (and may be null when not positioned on a row).  Sub-
	**	classes may assume that these values are consistent and 
	**	correct when load() is called and that load() will only be 
	**	called when conditions are valid for loading the next row 
	**	(pos_status will not be POS_CLOSED or POS_AFTER).
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
	**	or type.  
	**
	**	Sub-classes may override columnMap() and getSqlData() if they
	**	provide storage for column data objects which are outside the
	**	provided column data array.
	**
	**
	**  Interface Methods:		(other methods implemented as stubs/covers)
	**
	**	next			Move to next row in the result set.
	**	close			Close the result set and free resources.
	**	getStatement		Retrieve associated Statement.
	**	getType			Retrieve result set type.
	**	getConcurrency		Retrieve result set concurrency.
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
	**	findColumn		Determine column index from column name.
	**	wasNull			Determine if last column read was NULL.
	**	getBoolean		Retrieve column as a boolean value.
	**	getByte			Retrieve column as a byte value.
	**	getShort		Retrieve column as a short value.
	**	getInt			Retrieve column as a int value.
	**	getLong			Retrieve column as a long value.
	**	getFloat		Retrieve column as a float value.
	**	getDouble		Retrieve column as a double value.
	**	getDecimal		Retrieve column as a BigDecimal value.
	**	getString		Retrieve column as a string value.
	**	getBytes		Retrieve column as a byte array value.
	**	getDate			Retrieve column as a Date value.
	**	getTime			Retrieve column as a Time value.
	**	getTimestamp		Retrieve column as a Timestamp value.
	**	getBinaryStream		Retrieve column as a binary byte stream.
	**	getAsciiStream		Retrieve column as an ASCII character stream.
	**	getUnicodeStream	Retrieve column as a Unicode character stream.
	**	getCharacterStream	Retrieve column as a Character stream.
	**	getObject		Retrieve column as a generic object.
	**
	**  Public Methods:
	**
	**	shut			Close result-set (driver internal).
	**
	**  Abstract Methods:
	**
	**	load			Load data subset.
	**
	**  Constants:
	**
	**	POS_CLOSED		Result set closed.
	**	POS_ON_ROW		Positioned on row.
	**	POS_BEFORE		Positioned prior to first row.
	**	POS_AFTER		Positioned after last row.
	**
	**	ROW_IS_FIRST		Row is first row.
	**	ROW_IS_LAST		Row is last row.
	**	ROW_UPDATED		Row has been updated.
	**	ROW_DELETED		Row has been deleted.
	**
	**  Protected Data:
	**
	**	rsmd			Result set meta-data.
	**	pos_status		Position status.
	**	row_status		Status of current row.
	**	row_number		Current row number.
	**	columns			Array of column value objects.
	**	rs_type			Result-set type.
	**	rs_concur		Result-set concurrency.
	**	rs_fetch_dir		Fetch direction.
	**	rs_fetch_size		Pre-fetch size.
	**	rs_max_rows		Max rows to return.
	**	rs_max_len		Max column string length.
	**
	**  Protected Methods:
	**
	**	columnMap		Validates column index, convert to internal.
	**	getSqlData		Retrieve column data value.
	**
	**  Private Data:
	**
	**	isNull			Was last column retrieved NULL.
	**
	**  Private Methods:
	**
	**	columnByName		Determine column index from column name.
	**	columnDataValue		Generic column data access.
	**
	** History:
	**	14-May-99 (gordy)
	**	    Created.
	**	 8-Sep-99 (gordy)
	**	    Created new base class for class which interact with
	**	    the server and extracted common data and methods.
	**	    Synchronize entire request/response with server.
	**	29-Sep-99 (gordy)
	**	    Implemented support for BLOB data and added support
	**	    methods and fields.
	**	11-Nov-99 (gordy)
	**	    Extracted base functionality from AdvanResult.
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
	**	    Support 2.0 extensions.  Added stmt, rs_type, rs_concur,
	**	    rs_fetch_dir, rs_fetch_size, finalize(), getStatement(), getType(), 
	**	    getConcurrency(), getFetchDirection(), setFetchDirection(), 
	**	    getFetchSize(), setFetchSize(), getBigDecimal(), getDate(),
	**	    getTime(), getTimestamp(), getCharacterStream(), getObject(), 
	**	    getArray(), getBlob(), getClob(), getRef(), cursor positioning
	**	    and cursor update methods.  Renamed max_col_len to rs_max_len 
	**	    and max_row_cnt to rs_max_rows.
	**	23-Jan-01 (gordy)
	**	    The associated statement reference must be an AdvanStmt class
	**	    not a Statement interface.  Since
	**	    the AdvanStmt class implements these methods, they will be
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
	**	    UCS-2 character arrays into UTF-8 byte format.  2.1 spec
	**	    requires unicode streams to be UTF-8 encoded.
	**      29-Aug-01 (loera01) SIR 105641
	**          Made class AdvanStmt variable protected instead of private, so that
	**          subclass RsltProc can have access.
	**	26-Aug-02 (thoda04)
	**	    Ported for .NET Provider.
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
	*/

	abstract class AdvanRslt : DrvObj
	{

		/*
		** Position status values, only one valid at any time.
		*/
		protected const int POS_CLOSED    = -1;
		protected const int POS_ON_ROW    =  0;
		protected const int POS_BEFORE    =  1;
		protected const int POS_AFTER      = 2;

		/*
		** Row status flags, multiple valid for a given row.
		*/
		protected const int ROW_IS_FIRST   = 0x01;
		protected const int ROW_IS_LAST    = 0x02;
		protected const int ROW_UPDATED    = 0x04;
		protected const int ROW_DELETED    = 0x08;

		/*
		** Current row information.
		*/
		protected internal AdvanRSMD rsmd = null; // Result set meta-data.
		protected internal int	    pos_status = POS_BEFORE;	// Current position.
		protected internal int	    row_status = 0;	// Current row status flags.
		protected internal int	    row_number = 0;	// Current row number.
		protected internal SqlData[]	columns = null;	// Current row column data.
	
		/*
		** ResultSet attributes and application settings.
		*/
		protected int	    rs_concur =    DrvConst.DRV_CRSR_READONLY;
		protected int	    rs_fetch_dir = FETCH_FORWARD;
		protected int	    rs_fetch_size = 0;	// Pre-fetch size
		protected int	    rs_max_rows = 0;	// Max rows to return.
		protected int	    rs_max_len = 0;	// Max string length to return.
    
		/*
		** Local data.
		*/
		private bool	    isNull = false;    // Was last column NULL?


		/*
		** Name: load
		**
		** Description:
		**	Position the result set to the next row and load the column
		**	values.  
		**	
		**	If the next row is valid, the following fields must be set: 
		**	pos_status to POS_ON_ROW, row_status to indicate the status 
		**	of the row, row_number to the number of the row in the result 
		**	set, and columns to the column values for the row.
		**
		**	Loading of column values can be delayed until the point
		**	accessed by overriding the columnMap() method and loading
		**	the column value when accessed.
		**
		**	If the next row in invalid, the following must be set:
		**	pos_status to POS_AFTER or POS_CLOSED, row_status to 0,
		**	row_number to one greater than the number of rows, and
		**	columns to null.
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

		protected internal abstract void load();


		/*
		** Name: AdvanRslt
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
			AdvanRslt( DrvConn conn, AdvanRSMD rsmd ) : base( conn )
		{
			this.rsmd = rsmd;
			title = trace.getTraceName() + "-ResultSet[" + inst_id + "]";
			tr_id = "Rslt[" + inst_id + "]";
			return;
		} // AdvanRslt


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

/*
		protected void
			finalize()
		{
			if ( pos_status != POS_CLOSED ) 
				try { shut(); } 
				catch( Exception ignore ) {}
			base.finalize();
			return;
		} // finalize
*/


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
		**	 4-Aug-03 (gordy)
		**	    Moved pre-fetch row handling to sub-classes.
		*/

		public bool
			next()
		{
			lock(this)
			{
				bool ready = false;

				if ( trace.enabled() )  trace.log( title + ".next()" );
				warnings = null;
				isNull = false;

				/*
				** Result-set must be opened, not at EOD, and must be
				** within max row limit (if set).
				*/
				switch( pos_status )
				{
					case POS_CLOSED :
						if ( trace.enabled( 3 ) )
							trace.write( tr_id + ".next: result-set closed" );
						break;
	
					case POS_AFTER :
						if ( trace.enabled( 3 ) )
							trace.write( tr_id + ".next: result-set at EOD" );
						break;
	
					default :
						/*
						** Has the max row limit been reached?
						*/
						if ( rs_max_rows > 0  &&  row_number >= rs_max_rows )
						{
							try { shut(); }  
							catch( SqlEx ) {}
							if ( trace.enabled( 3 ) )
								trace.write( tr_id + ".next: max row limit reached" );
							break;
						}
	
						/*
						** Request the next row be loaded by sub-class.
						*/
						try { load(); }
						catch( SqlEx ex )
						{
							if ( trace.enabled() )  
								trace.log( title + ".next(): error loading row" );
							if ( trace.enabled( 1 ) )  ex.trace( trace );
							try { shut(); }  
							catch( SqlEx ) {}
							throw ex;
						}
	
						ready = (pos_status == POS_ON_ROW);	// Was a row loaded?
						break;
				}

				if ( trace.enabled() )  trace.log( title + ".next: " + ready );
				return( ready );
			}
		} // next


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
		{
			if ( trace.enabled() )  trace.log( title + ".close()" );
			warnings = null;
			isNull = false;

			try 
			{ 
				if ( ! shut()  &&  trace.enabled( 5 ) )  
					trace.write( tr_id + ": result-set already closed!" );
			}
			catch( SqlEx ex )
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
		**	called (super.shut()) to set the result set status info.
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
		**	 4-Aug-03 (gordy)
		**	    Replaced row cache with status info.
		*/

		internal virtual bool shut()
		{
			lock( this )
			{
				if ( pos_status == POS_CLOSED )  return( false );
				pos_status = POS_CLOSED;
			}

			if ( trace.enabled( 3 ) )  trace.write( tr_id + ": result-set closed" );
			row_status = 0;
			row_number = 0;
			columns = null;
			return( true );
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
		**	31-Oct-02 (gordy)
		**	    Removed statement info since only some sub-classes are
		**	    associated with statements.
		*/

//		public Statement
//			getStatement()
//		{
//			if ( trace.enabled() )  trace.log( title + ".getStatement(): " + null );
//			return( null );
//		} // getStatement


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

		internal int
			getConcurrency()
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
/*
		public void
			setFetchDirection( int direction )
		{
			if ( trace.enabled() )  
				trace.log( title + ".setFetchDirection( " + direction + " )" );
			if ( direction != ResultSet.FETCH_FORWARD )
				throw SqlEx.get( ERR_GC4010_PARAM_VALUE );
			return;
		} // setFetchDirection
*/

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
		{
			if ( trace.enabled() )  
				trace.log( title + ".setFetchSize( " + size + " )" );
			if ( size < 0  ||  (rs_max_rows > 0  &&  size > rs_max_rows) )
				throw SqlEx.get( ERR_GC4010_PARAM_VALUE );
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

		internal AdvanRSMD
			getMetaData()
		{
			if ( trace.enabled() )  trace.log( title + ".getMetaData(): " + rsmd );
			if( pos_status == POS_CLOSED ) 
				throw SqlEx.get( ERR_GC401D_RESULTSET_CLOSED );
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
		{
			int row = (pos_status != POS_ON_ROW) ? 0 : row_number;
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
		**	does not satisfy the specification in regards to empty
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

		public bool
			isBeforeFirst()
		{
			bool val = (pos_status == POS_BEFORE);
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
		**	bool	    TRUE if after last, FALSE otherwise.
		**
		** History:
		**	 6-Nov-00 (gordy)
		**	    Created.
		*/

		public bool
			isAfterLast()
		{
			bool val = (pos_status == POS_AFTER  &&  row_number > 1);
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
		**	bool	    TRUE if on first row, FALSE otherwise.
		**
		** History:
		**	 6-Nov-00 (gordy)
		**	    Created.
		*/

		public bool
			isFirst()
		{
			bool val = ( pos_status == POS_ON_ROW  && 
				(row_status & ROW_IS_FIRST) != 0 );
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
		**	This implementation does not satisfy the 
		**	specification.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	bool	    FALSE (can't determine required state).
		**
		** History:
		**	 6-Nov-00 (gordy)
		**	    Created.
		*/

		public bool
			isLast()
		{
			bool val = ( pos_status == POS_ON_ROW  && 
				(row_status & ROW_IS_LAST) != 0 );
			if ( trace.enabled() )  trace.log( title + ".isLast(): " + false );
			return( false );
		} // isLast


		/*
		** Cursor positioning methods (not supported at this level).
		*/

		public void 
			beforeFirst()
		{ 
			if ( trace.enabled() )  trace.log( title + ".beforeFirst()" );
			throw( SqlEx.get( ERR_GC4019_UNSUPPORTED ) ); 
		}

		public void 
			afterLast()
		{ 
			if ( trace.enabled() )  trace.log( title + ".afterLast()" );
			throw( SqlEx.get( ERR_GC4019_UNSUPPORTED ) ); 
		} 

		public bool 
			first()
		{ 
			if ( trace.enabled() )  trace.log( title + ".first()" );
			throw( SqlEx.get( ERR_GC4019_UNSUPPORTED ) ); 
		}

		public bool 
			last()
		{ 
			if ( trace.enabled() )  trace.log( title + ".last()" );
			throw( SqlEx.get( ERR_GC4019_UNSUPPORTED ) ); 
		}

		public bool 
			previous()
		{ 
			if ( trace.enabled() )  trace.log( title + ".previous()" );
			throw( SqlEx.get( ERR_GC4019_UNSUPPORTED ) ); 
		}

		public bool 
			absolute( int row )
		{ 
			if ( trace.enabled() )  trace.log( title + ".absolute( " + row + " )" );
			throw( SqlEx.get( ERR_GC4019_UNSUPPORTED ) ); 
		}

		public bool 
			relative( int rows )
		{ 
			if ( trace.enabled() )  trace.log( title + ".relative( " + rows + " )" );
			throw( SqlEx.get( ERR_GC4019_UNSUPPORTED ) ); 
		}


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
		**	bool		True if row updated, false otherwise.
		**
		** History:
		**	 4-Aug-03 (gordy)
		**	    Implemented using row status info.
		*/

		public bool 
			rowUpdated()
		{ 
			bool updated = ( pos_status == POS_ON_ROW  &&
				(row_status & ROW_UPDATED) != 0 );
			if ( trace.enabled() )  trace.log( title + ".rowUpdated(): " + updated );
			return( updated ); 
		}

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
		**	bool		True if row deleted, false otherwise.
		**
		** History:
		**	 4-Aug-03 (gordy)
		**	    Implemented using row status info.
		*/

		public bool 
			rowDeleted()
		{ 
			bool deleted = ( pos_status == POS_ON_ROW  &&
				(row_status & ROW_DELETED) != 0 );
			if ( trace.enabled() )  trace.log( title + ".rowDeleted(): " + deleted );
			return( deleted ); 
		}


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
		**	bool		True if row inserted, false otherwise.
		**
		** History:
		**	 4-Aug-03 (gordy)
		**	    Implemented using row status info.
		*/

		public bool 
			rowInserted()
		{ 
			if ( trace.enabled() )  trace.log( title + ".rowInserted(): " + false );
			return( false ); 
		}

		/*
		** Cursor update methods (not supported at this level).
		*/

		public void 
			insertRow()
		{ 
			if ( trace.enabled() )  trace.log( title + ".insertRow()" );
			throw SqlEx.get( ERR_GC4019_UNSUPPORTED ); 
		}

		public void 
			updateRow()
		{ 
			if ( trace.enabled() )  trace.log( title + ".updateRow()" );
			throw SqlEx.get( ERR_GC4019_UNSUPPORTED ); 
		}

		public void 
			deleteRow()
		{ 
			if ( trace.enabled() )  trace.log( title + ".deleteRow()" );
			throw SqlEx.get( ERR_GC4019_UNSUPPORTED ); 
		}

		public void 
			refreshRow()
		{ 
			if ( trace.enabled() )  trace.log( title + ".refreshRow()" );
			throw SqlEx.get( ERR_GC4019_UNSUPPORTED ); 
		}

		public void 
			moveToInsertRow()
		{ 
			if ( trace.enabled() )  trace.log( title + ".moveToInsertRow()" );
			throw SqlEx.get( ERR_GC4019_UNSUPPORTED ); 
		}

		public void 
			moveToCurrentRow()
		{ 
			if ( trace.enabled() )  trace.log( title + ".moveToCurrentRow()" );
			throw SqlEx.get( ERR_GC4019_UNSUPPORTED ); 
		}


		/*
		** Cursor update methods (not implemented at this level).
		*/

		public void
			cancelRowUpdates()
		{ if ( trace.enabled() )  trace.log(title + ".cancelRowUpdates(): " + false); }

		public void
			updateNull( int index )
		{ 
			if ( trace.enabled() ) 
				trace.log( title + ".updateNull( " + index + " )" ); 
		}

		public void
			updateBoolean( int index, bool value )
		{ 
			if ( trace.enabled() )  
				trace.log(title + ".updateBoolean(" + index + "," + value + ")");
		}

		public void
			updateByte( int index, byte value )
		{ 
			if ( trace.enabled() )  
				trace.log(title + ".updateByte(" + index + "," + value + ")");
		}

		public void
			updateShort( int index, short value )
		{ 
			if ( trace.enabled() )  
				trace.log(title + ".updateShort(" + index + "," + value + ")");
		}

		public void
			updateInt( int index, int value )
		{ 
			if ( trace.enabled() )  
				trace.log(title + ".updateInt(" + index + "," + value + ")");
		}

		public void
			updateLong( int index, long value )
		{ 
			if ( trace.enabled() )  
				trace.log(title + ".updateLong(" + index + "," + value + ")");
		}

		public void
			updateFloat( int index, float value )
		{ 
			if ( trace.enabled() )  
				trace.log(title + ".updateFloat(" + index + "," + value + ")");
		}

		public void
			updateDouble( int index, double value )
		{ 
			if ( trace.enabled() )  
				trace.log(title + ".updateDouble(" + index + "," + value + ")");
		}

		public void
			updateDecimal( int index, Decimal value )
		{ 
			if ( trace.enabled() )  
				trace.log( title + ".updateDecimal(" + index + "," + value + ")" );
		}

		public void
			updateString( int index, String value )
		{ 
			if ( trace.enabled() )  
				trace.log(title + ".updateString(" + index + "," + value + ")");
		}

		public void
			updateBytes( int index, byte[] value )
		{ 
			if ( trace.enabled() )  
				trace.log(title + ".updateBytes(" + index + "," + value + ")");
		}

		public void
			updateDate( int index, DateTime value )
		{ 
			if ( trace.enabled() )  
				trace.log(title + ".updateDate(" + index + "," + value + ")");
		}

		public void
			updateTime( int index, TimeSpan value )
		{ 
			if ( trace.enabled() )  
				trace.log(title + ".updateTime(" + index + "," + value + ")");
		}

		public void
			updateTimestamp( int index, DateTime value )
		{ 
			if ( trace.enabled() )  trace.log( title + ".updateTimestamp(" + 
										index + "," + value + ")" );
		}

		public void
			updateBinaryStream( int index, InputStream inStream, int length ) 
		{ 
			if ( trace.enabled() )  trace.log( title + ".updateBinaryStream(" + 
										index + "," + length + ")" );
		}

		public void
			updateAsciiStream( int index, InputStream inStream, int length ) 
		{ 
			if ( trace.enabled() )  trace.log( title + ".updateAsciiStream(" + 
										index + "," + length + ")");
		}

		public void
			updateCharacterStream( int index, Reader inReader, int length ) 
		{ 
			if ( trace.enabled() )  trace.log( title + ".updateCharacterStream(" + 
										index + "," + length + ")" );
		}

		public void 
			updateObject( int index, Object value, int scale ) 
		{ 
			if ( trace.enabled() )  trace.log( title + ".updateObject(" + 
										index + "," + value + "," + scale + ")" );
		}


		/*
		** Data update methods which are simple covers for
		** the primary data update methods.
		*/

		public void 
			updateNull( String name )
		{ updateNull(columnByName(name)); }

		public void 
			updateBoolean( String name, bool value )
		{ updateBoolean(columnByName(name), value); }

		public void 
			updateByte( String name, byte value )
		{ updateByte(columnByName(name), value); }

		public void 
			updateShort( String name, short value )
		{ updateShort(columnByName(name), value); }

		public void 
			updateInt( String name, int value )
		{ updateInt( columnByName( name ), value ); }

		public void 
			updateLong( String name, long value )
		{ updateLong( columnByName( name ), value ); }

		public void 
			updateFloat( String name, float value )
		{ updateFloat( columnByName( name ), value ); }

		public void 
			updateDouble( String name, double value )
		{ updateDouble( columnByName( name ), value ); }

		public void 
			updateDecimal( String name, Decimal value )
		{ updateDecimal( columnByName( name ), value ); }

		public void 
			updateString( String name, String value )
		{ updateString( columnByName( name ), value ); }

		public void 
			updateBytes( String name, byte[] value )
		{ updateBytes( columnByName( name ), value ); }

		public void 
			updateDate( String name, DateTime value )
		{ updateDate( columnByName( name ), value ); }

		public void 
			updateTime( String name, TimeSpan value )
		{ updateTime( columnByName( name ), value ); }

		public void 
			updateTimestamp( String name, DateTime value )
		{ updateTimestamp( columnByName( name ), value ); }

		public void 
			updateBinaryStream( String name, InputStream inStream, int length ) 
		{ updateBinaryStream( columnByName( name ), inStream, length ); }

		public void 
			updateAsciiStream( String name, InputStream inStream, int length ) 
		{ updateAsciiStream( columnByName( name ), inStream, length ); }

		public void 
			updateCharacterStream( String name, Reader inReader, int length ) 
		{ updateCharacterStream( columnByName( name ), inReader, length ); }

		public void 
			updateObject( int index, Object value )
		{ updateObject( index, value, -1 ); }

		public void 
			updateObject( String name, Object value )
		{ updateObject( columnByName( name ), value, -1 ); }

		public void 
			updateObject( String name, Object value, int scale ) 
		{ updateObject( columnByName( name ), value, scale ); }


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
		{
			if ( trace.enabled() )  trace.log( title + ".findColumn('" + name + "')" );
			int col;
    
			try { col = columnByName( name ); }
			catch( SqlEx ex )
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

		public bool
			wasNull()
		{
			if ( trace.enabled() )  trace.log( title + ".wasNull(): " + isNull );
			return( isNull );
		} // wasNull


		/*
		** Name: IsDBNull
		**
		** Description:
		**	Determine if column to be retrieved is NULL.
		**
		** Input:
		**	columnIndex Column index.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	    True if column is NULL, false otherwise.
		**
		** History:
		**	 1-Nov-02 (thoda04)
		**	    Created.*/
		
		public bool IsDBNull(int index)
		{
			if ( trace.enabled() )  trace.log( title + ".IsDBNull( " + index + " )" );

			SqlData	data = columnDataValue( index );
			bool	value = data.isNull();

			if ( trace.enabled() )  trace.log( title + ".IsDBNull: " + value );
			return( value );

		}  // IsDBNull


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
		**	    Added BOOLEAN data type, treat same as BIT.
		**	26-Sep-03 (gordy)
		**	    Raw data values replaced with SqlData objects.
		*/

		public bool
			getBoolean( int index )
		{
			if ( trace.enabled() )  trace.log( title + ".getBoolean( " + index + " )" );

			SqlData data = columnDataValue(index);
			bool	value = data.isNull() ? false : data.getBoolean();

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
		**	    Added BOOLEAN data type, treat same as BIT.
		**	26-Sep-03 (gordy)
		**	    Raw data values replaced with SqlData objects.
		*/

		public byte
			getByte( int index )
		{
			if ( trace.enabled() )  trace.log( title + ".getByte( " + index + " )" );

			SqlData data = columnDataValue(index);
			byte value = data.isNull() ? (byte)0 : data.getByte();

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
		**	    Added BOOLEAN data type, treat same as BIT.
		**	26-Sep-03 (gordy)
		**	    Raw data values replaced with SqlData objects.
		*/

		public short
			getShort( int index )
		{
			if ( trace.enabled() )  trace.log( title + ".getShort( " + index + " )" );

			SqlData data = columnDataValue(index);
			short value = data.isNull() ? (short)0 : data.getShort();

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
		**	    Added BOOLEAN data type, treat same as BIT.
		**	26-Sep-03 (gordy)
		**	    Raw data values replaced with SqlData objects.
		*/

		public int
			getInt( int index )
		{
			if ( trace.enabled() )  trace.log( title + ".getInt( " + index + " )" );

			SqlData data = columnDataValue(index);
			int value = data.isNull() ? 0 : data.getInt();

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
		**	    added BOOLEAN data type, treat same as BIT.
		**	26-Sep-03 (gordy)
		**	    Raw data values replaced with SqlData objects.
		*/

		public long
			getLong( int index )
		{
			if ( trace.enabled() )  trace.log( title + ".getLong( " + index + " )" );

			SqlData data = columnDataValue(index);
			long value = data.isNull() ? 0L : data.getLong();

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
		**	    Added BOOLEAN data type, treat same as BIT.
		**	26-Sep-03 (gordy)
		**	    Raw data values replaced with SqlData objects.
		*/

		public float
			getFloat( int index )
		{
			if ( trace.enabled() )  trace.log( title + ".getFloat( " + index + " )" );

			SqlData data = columnDataValue(index);
			float value = data.isNull() ? 0.0F : data.getFloat();

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
		**	    Added BOOLEAN data type, treat same as BIT.
		**	26-Sep-03 (gordy)
		**	    Raw data values replaced with SqlData objects.
		*/

		public double
			getDouble( int index )
		{
			if ( trace.enabled() )  trace.log( title + ".getDouble( " + index + " )" );

			SqlData data = columnDataValue(index);
			double value = data.isNull() ? 0.0 : data.getDouble();

			if ( trace.enabled() )  trace.log( title + ".getDouble: " + value );
			return( value );
		} // getDouble


		/*
		** Name: getDecimal
		**
		** Description:
		**	Retrieve column data as a Decimal value.  If the column
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
		**	Decimal  Column numeric value.
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
		**	    Added BOOLEAN data type, treat same as BIT.
		**	26-Sep-03 (gordy)
		**	    Raw data values replaced with SqlData objects.
		*/

		public Decimal
			getDecimal( int index, int scale )
		{
			if ( trace.enabled() )  trace.log( title + ".getDecimal( " + 
										index + ", " + scale + " )" );

			SqlData data = columnDataValue(index);
			Decimal value = data.isNull() ? Decimal.MinValue : data.getDecimal();

			if ( trace.enabled() )
				trace.log( title + ".getDecimal: " + value );
			return( value );
		} // getDecimal


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
		**	    Added BOOLEAN data type, treat same as BIT.
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
		{
			if ( trace.enabled() )  trace.log( title + ".getString( " + index + " )" );

			SqlData data = columnDataValue(index);
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
			if ( data is IngresDate  &&  ((IngresDate)data).isInterval() )
			{
				/*
				** A truncation warning was most likely generated
				** because of the interval, so drop that warning
				** prior to generating the interval warning.
				*/
				warnings = null;
				setWarning( SqlEx.get( ERR_GC401B_INVALID_DATE ) );
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
		*/

		public byte[]
			getBytes( int index )
		{
			if ( trace.enabled() )  trace.log( title + ".getBytes( " + index + " )" );

			SqlData data = columnDataValue(index);
			byte[]	value;

			if (data.isNull())
				value = null;
			else if (rs_max_len > 0)
				value = data.getBytes(rs_max_len);
			else
				value = data.getBytes();

			if ( trace.enabled() )  trace.log( title + ".getBytes: " + value );
			return( value );
		} // getBytes


		/*
		** Name: getDate
		**
		** Description:
		**	Retrieve column data as a Date value.  If the column
		**	is NULL, null is returned.  Dates extracted from strings
		**	must be date escape format: YYYY-MM-DD.
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
		*/

		public DateTime
			getDate( int index )
		{
			if ( trace.enabled() )  trace.log( title + ".getDate( " + 
										index + " )" );

			SqlData data = columnDataValue(index);
			DateTime value = data.isNull() ? DateTime.MinValue : 
				data.getDate( null );

			if ( trace.enabled() )  trace.log( title + ".getDate: " + value );
			return( value );
		} // getDate


		/*
		** Name: getTime
		**
		** Description:
		**	Retrieve column data as a Time value.  If the column
		**	is NULL, null is returned.  Times extracted from strings
		**	must be in time escape format: HH:MM:SS.
		**
		**	When the timezone value of a time value is not known (from
		**	gateways, Ingres is always GMT), the timezone from the
		**	Calendar parameter is used (local timezone if cal is NULL).
		**
		**	The Date value produced should have the date component set
		**	to the date EPOCH: 1970-01-01.
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

		public DateTime
			getTime( int index )
		{
			if ( trace.enabled() )  trace.log( title + ".getTime( " + 
										index + " )" );

			SqlData data = columnDataValue(index);
			DateTime value = data.isNull() ? new DateTime(1970, 1, 1) : 
				data.getTime( null );

			if ( trace.enabled() )  trace.log( title + ".getTime: " + value );
			return( value );
		} // getTime


		/*
		** Name: getTimestamp
		**
		** Description:
		**	Retrieve column data as a Timestamp value.  If the column
		**	is NULL, null is returned.  Timestamps extracted from strings
		**	must be in timestamp escape format: YYYY-MM-DD HH:MM:SS.
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

		public DateTime
			getTimestamp( int index )
		{
			if ( trace.enabled() )  trace.log( title + ".getTimestamp( " + 
										index + " )" );

			SqlData data = columnDataValue(index);
			DateTime value = data.isNull() ? DateTime.MinValue : 
				data.getTimestamp( null );

			if ( trace.enabled() )  trace.log( title + ".getTimestamp: " + value );
			return( value );
		} // getTimestamp


		/*
		** Name: getTimeSpan
		**
		** Description:
		**	Retrieve column data as a TimeSpan value.  If the column
		**	is NULL, null is returned.
		**
		** Input:
		**	index	    Column index.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	TimeSpan    Column TimeSpan value.
		**
		** History:
		**	25-Oct-06 (thoda04)
		**	    Created.
		*/

		public TimeSpan
			getTimeSpan(int index)
		{
			if (trace.enabled()) trace.log(title + ".getTimeSpan( " +
									 index + " )");

			SqlData data = columnDataValue(index);
			TimeSpan value = data.isNull() ? new TimeSpan(0) :
				data.getTimeSpan();

			if (trace.enabled()) trace.log(title + ".getTimeSpan: " + value);
			return (value);
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
		{
			SqlData data;
			InputStream value;

			if (trace.enabled())
				trace.log(title + ".getBinaryStream( " + index + " )");

			data = columnDataValue(index);

			try { value = data.isNull() ? null : data.getBinaryStream(); }
			catch (SqlEx ex)
			{
				if (trace.enabled())
					trace.log(title + ".getBinaryStream: error accessing BLOB data");
				if (trace.enabled(1)) ex.trace(trace);
				throw ex;
			}

			if (trace.enabled()) trace.log(title + ".getBinaryStream: " + value);
			return (value);
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
		{
			SqlData data;
			InputStream value;

			if (trace.enabled())
				trace.log(title + ".getAsciiStream( " + index + " )");

			data = columnDataValue(index);

			try { value = data.isNull() ? null : data.getAsciiStream(); }
			catch (SqlEx ex)
			{
				if (trace.enabled())
					trace.log(title + ".getAsciiStream: error accessing BLOB data");
				if (trace.enabled(1)) ex.trace(trace);
				throw ex;
			}

			if (trace.enabled()) trace.log(title + ".getAsciiStream: " + value);
			return (value);
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
		{
			SqlData data;
			InputStream value;

			if (trace.enabled())
				trace.log(title + ".getUnicodeStream( " + index + " )");

			data = columnDataValue(index);

			try { value = data.isNull() ? null : data.getUnicodeStream(); }
			catch (SqlEx ex)
			{
				if (trace.enabled())
					trace.log(title + ".getUnicodeStream: error accessing BLOB data");
				if (trace.enabled(1)) ex.trace(trace);
				throw ex;
			}

			if (trace.enabled()) trace.log(title + ".getUnicodeStream: " + value);
			return (value);
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
		{
			SqlData data;
			Reader value;

			if (trace.enabled())
				trace.log(title + ".getCharacterStream( " + index + " )");

			data = columnDataValue(index);

			try { value = data.isNull() ? null : data.getCharacterStream(); }
			catch (SqlEx ex)
			{
				if (trace.enabled())
					trace.log(title + ".getCharacterStream: error accessing BLOB data");
				if (trace.enabled(1)) ex.trace(trace);
				throw ex;
			}

			if (trace.enabled()) trace.log(title + ".getCharacterStream: " + value);
			return (value);
		} // getCharacterStream


		/*
		** Name: getObject
		**
		** Description:
		**	Retrieve column data as a generic object.  If the column
		**	is NULL, null is returned.
		**
		** Input:
		**	index	Column index.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Object	object.
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
		{
			if (trace.enabled()) trace.log(title + ".getObject( " + index + " )");

			SqlData data = columnDataValue(index);
			Object value;

			if (data.isNull())
				value = null;
			else if (rs_max_len > 0)
				value = data.getObject(rs_max_len);
			else
				value = data.getObject();

			if (trace.enabled()) trace.log(title + ".getObject: " + value);
			return (value);
		} // getObject


		/*
		** Data retrieve methods which are for unsupported types
		*/

		public Array
			getArray( int index )
		{
			if (trace.enabled())  trace.log(title + ".getArray( " + index + " )");
			throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
		} // getArray



		/*
		** Data access methods which are simple covers for
		** the primary data access method.
		*/

		public bool
			getBoolean( String name )
		{ return( getBoolean( columnByName( name ) ) ); } 

		public byte
			getByte( String name )
		{ return( getByte( columnByName( name ) ) ); } 

		public short
			getShort( String name )
		{ return( getShort( columnByName( name ) ) ); }

		public int
			getInt( String name )
		{ return( getInt( columnByName( name ) ) ); }

		public long
			getLong( String name )
		{ return( getLong( columnByName( name ) ) ); }

		public float
			getFloat( String name )
		{ return( getFloat( columnByName( name ) ) ); }

		public double
			getDouble( String name )
		{ return( getDouble( columnByName( name ) ) ); }

		public Decimal
			getDecimal( String name, int scale )
		{ return( getDecimal( columnByName( name ), scale ) ); }

		public Decimal
			getDecimal( int index )
		{ return( getDecimal( index, -1 ) ); }

		public Decimal
			getDecimal( String name )
		{ return( getDecimal( name, -1 ) ); }

		public String
			getString( String name )
		{ return( getString( columnByName( name ) ) ); }

		public byte[]
			getBytes( String name )
		{ return( getBytes( columnByName( name ) ) ); }

		public DateTime
			getDate( String name )
		{ return( getDate( columnByName( name ) ) ); }

		public DateTime
			getTime( String name )
		{ return( getTime( columnByName( name ) ) ); }

		public DateTime
			getTimestamp( String name )
		{ return( getTimestamp( columnByName( name ) ) ); }

		public InputStream
			getBinaryStream( String name )
		{ return( getBinaryStream( columnByName( name ) ) ); }

		public InputStream
			getAsciiStream( String name )
		{ return( getAsciiStream( columnByName( name ) ) ); }

		public InputStream
			getUnicodeStream( String name )
		{ return( getUnicodeStream( columnByName( name ) ) ); }

		public Reader
			getCharacterStream( String name )
		{ return( getCharacterStream( columnByName( name ) ) ); }

		public Array
			getArray( String name )
		{ return( getArray( columnByName( name ) ) ); }

		public Object 
			getObject( String name )
		{ return( getObject( columnByName( name ) ) ); }


		/*
		** Name: columnMap
		**
		** Description:
		**	Determines if a given column index is a part of the result
		**	set and maps the external index to the internal index.
		**
		** Input:
		**	index	External column index [0,rsmd.count - 1].
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
		*/

		protected virtual int
			columnMap( int index )
		{
			if ( pos_status == POS_CLOSED ) 
				throw SqlEx.get( ERR_GC401D_RESULTSET_CLOSED );

			if (pos_status != POS_ON_ROW || (row_status & ROW_DELETED) != 0)
				throw SqlEx.get(ERR_GC4021_INVALID_ROW);

			if (index < 0 || index >= rsmd.count )
				throw SqlEx.get( ERR_GC4011_INDEX_RANGE );

			return( index );	// Internally, column indexes start at 0.
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
		{
			return( columns[ index ] );
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
		{
			for (int col = 0; col < rsmd.count; col++)
				if (rsmd.desc[col].name != null &&
					ToInvariantLower(rsmd.desc[col].name).Equals(
					ToInvariantLower(name)))
				{
					if (trace.enabled(5))
						trace.log(title + ": column '" + name + "' => " + col);
					return (col);
				}

			if (trace.enabled(5))
				trace.log(title + ": column '" + name + "' not found!");
			throw SqlEx.get(ERR_GC4012_INVALID_COLUMN_NAME);
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

		private SqlData
		columnDataValue( int index )
		{
			lock(this)
			{
				warnings = null;
				SqlData data = getSqlData( columnMap( index ) );
				isNull = data.isNull();

				if ( ! isNull  &&  data.isTruncated() )  
				setWarning( DataTruncation( index, false, true, 
					data.getDataSize(), 
					data.getTruncSize() ) );
				return( data );
			}
		} // columnDataValue


	} // class AdvanRslt

}  // namespace
