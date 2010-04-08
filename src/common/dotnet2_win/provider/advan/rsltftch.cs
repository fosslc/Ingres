/*
** Copyright (c) 1999, 2009 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.Collections;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: rsltftch.cs
	**
	** Description:
	**	Defines class which implements the ResultSet interface
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
	**	    the server and extracted common data and methods.
	**	    Synchronize entire request/response with server.
	**	13-Sep-99 (gordy)
	**	    Implemented error code support.
	**	29-Sep-99 (gordy)
	**	    Added class to support BLOBs as streams.  Implemented
	**	    BLOB data support.  Use DbConn lock()/unlock() methods
	**	    for synchronization.
	**	11-Nov-99 (gordy)
	**	    Extracted base functionality to AdvanRslt.
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
	**	    Added support for 2.0 extensions.
	**	23-Jan-01 (gordy)
	**	    Changed parameter type to AdvanStmt for backward compatibility.
	**	31-Jan-01 (gordy)
	**	    Don't throw protocol exceptions for invalid date formats.
	**	    AdvanRslt now handles these as conversion errors.
	**	28-Mar-01 (gordy)
	**	    Combined functionality of close() into shut().  Renamed
	**	    shut() to closeCursor() and created new shut().
	**	 4-Apr-01 (gordy)
	**	    Empty Date DataTruncation warnings moved to AdvanRslt.
	**	18-Apr-01 (gordy)
	**	    Use readBytes() method which reads length from input message.
	**	10-May-01 (gordy)
	**	    Support UCS2 transport format for character datatypes.
	**	19-Jun-02 (gordy)
	**	    BUG 108286 - Protocol error closing result-set with blob column
	**	    which was not accessed with getXXX().  Changed the processing 
	**	    order in shut() to perform the local class close operations 
	**	    before calling the super class shut().
	**	 3-Sep-02 (thoda04)
	**	    Ported for .NET Provider.
	**	31-Oct-02 (gordy)
	**	    Adapted for generic GCF driver.
	**	20-Mar-03 (gordy)
	**	    Updated to 3.0.
	**	 4-Aug-03 (gordy)
	**	    Removed row cacheing from AdvanRslt and implemented as a
	**	    dynamically sized list.
	**	20-Aug-03 (gordy)
	**	    Check for automatic cursor closure.
	**	22-Sep-03 (gordy)
	**	    SQL data objects now encapsulate data values.
	**	 6-Oct-03 (gordy)
	**	    Added pre-loading ability to result-sets.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	14-dec-04 (thoda04) SIR 113624
	**	    Add BLANKDATE=NULL support.
	**	 8-feb-04 (thoda04) B113867
	**	    DB procedure Output and InputOutput parameter
	**	    values were not being returned.
	**	15-Mar-04 (gordy)
	**	    Support BIGINT columns.
	**	10-May-05 (gordy)
	**	    Don't throw exception when server has already closed cursor.
	**	16-Jun-06 (gordy)
	**	    ANSI Date/Time data type support.
	**	21-Mar-07 (thoda04)
	**	    Added IntervalDayToSecond, IntervalYearToMonth.
	**	 7-Dec-09 (gordy, ported by thoda04)
	**	    Added support for BOOLEAN data type.
	*/



	/*
	** Name: RsltFtch
	**
	** Description:
	**	driver class implementing the ResultSet interface
	**	for result data fetched from a server.  While not abstract,
	**	this class should be sub-classed to provide data source 
	**	specific support and connection locking.
	**
	**	This class supports both multi-row fetching (pre-fetch) and
	**	partial row fetches (for BLOB support), though the two are
	**	not expected to occur at the same time.  BLOB columns result 
	**	in partial row processing and a BLOB value is not available 
	**	once processing passes beyond the BLOB column.  Pre-fetch
	**	is automatically disabled for result-sets containing BLOBs.
	**
	**	Row pre-fetch is enabled by default (except when BLOBs are
	**	present in the result-set).  If sub-classes need to disable 
	**	pre-fetching, they may do so by calling disablePreFetch() 
	**	prior to the first fetch request.  When pre-fetching is 
	**	enabled, rows and column values are stored in a dynamically
	**	sized list: rowCache.  
	**
	**	When pre-fetching is disabled, column values are stored in 
	**	the column data array provided by the super-class.  The column 
	**	counter, column_count, is incremented for each column loaded.
	**
	**	Column processing is interrupted when a BLOB column is read.
	**	The column counter is set to indicate that the column has
	**	been loaded even though the actual contents of the BLOB
	**	stream remain to be read.  The active stream is saved and
	**	the reference, activeStream, may be used by sub-classes to
	**	check for active BLOB columns.  This class implements the
	**	SQL stream listener interface to receive notification of
	**	the stream closure.  Column processing continues via a
	**	call to the resume() method.  Active streams are forcefully
	**	closed (flush()) when unread columns following the BLOB are 
	**	referenced and for other requests which require server 
	**	communications.
	**
	**	This class does not provide connection locking support which
	**	is left as a responsibility of sub-classes.  Methods which 
	**	may require locking support are fetch(), resume(), and
	**	closeCursor().  The following conditions may be assumed when
	**	these methods are called: the connection is dormant when
	**	fetch() and closeCursor() are called, active when resume()
	**	is called.  The connection will be active if a stream is
	**	active when fetch() or resume() return.  Otherwise, the
	**	connection will be dormant upon exit from all three methods.
	**
	**  Interface Mehtods:
	**
	**	streamClosed		SQL Stream was closed.
	**
	**  Overridden Methods:
	**
	**	getStatement		Return associated statement.
	**	load			Load next row.
	**	shut			Close result-set.
	**	columnMap		Map column indexes.
	**	readData		Read data from server.
	**	setProcResult		Save procedure return value.
	**
	**  Protected Data:
	**
	**	cursorClosed		Has the cursor been closed.
	**	activeStream		Active BLOB stream.
	**
	**  Protected Methods:
	**
	**	disablePreFetch		Disable row pre-fetching.
	**	preLoad			Pre-load the row cache.
	**	fetch			Request and load rows/columns from DBMS.
	**	resume			Resume message processing after BLOB.
	**	closeCursor		Close cursor.
	**	flush			Flush partial rows.
	**
	**  Private Data:
	**
	**	stmt			Associated statement.
	**	stmt_id			DBMS statement ID.
	**	column_count		Number of columns available in current row.
	**	rowCacheEnabled		Is row cache and pre-fetching enabled.
	**	rowCache		Row cache.
	**	freeCache		Unused row cache.
	**
	**  Private Methods:
	**
	**	initRowCache		Initializes the row cache.
	**	loadRowCache		Load row cache with column data from server.
	**	allocateRowBuffer	Allocate column data array and column objects.
	**	readColumns		Read column data from server.
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
	**	    Extracted base functionality to AdvanRslt.
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
	**	 3-Sep-02 (thoda04)
	**	    Ported for .NET Provider.
	**	31-Oct-02 (gordy)
	**	    Super-class now indicates BLOB closure with blobClosed() method.
	**	    statement info no longer maintained by super-class.  Added
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
	*/

	class
		RsltFtch : AdvanRslt, SqlStream.IStreamListener
	{

		protected bool		cursorClosed = false;	// Is cursor closed.
		protected SqlStream		activeStream = null;	// Active BLOB stream.

		private AdvanStmt		stmt = null;		// Assoc. statement.
		private long		stmt_id = 0;		// Server statement ID.

		private int			column_count = 0;	// Available columns.
    
		private bool		rowCacheEnabled = true;	// Pre-fetch enabled?
		private ArrayList		rowCache = null;	// Row cache.
		private ArrayList		freeCache = null;	// Unused rows.

    
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
		**	stmt_id		Statement ID.
		**	preFetch	Pre-fetch row count.
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
		**	    Parameters changed for 2.0 extensions.
		**	23-Jan-01 (gordy)
		**	    Changed parameter type to AdvanStmt for backward compatibility.
		**	31-Oct-02 (gordy)
		**	    Adapted for generic GCF driver.  Statement info maintained here.
		**	 4-Aug-03 (gordy)
		**	    Pre-fetch handling moved entirely into this class.
		*/

		public
			RsltFtch
			( 
			DrvConn	conn,
			AdvanStmt	stmt, 
			AdvanRSMD	rsmd, 
			long	stmt_id,
			int		preFetch
			) : base( conn, rsmd )
		{
			this.stmt = stmt;
			this.stmt_id = stmt_id;
			rs_max_len = stmt.rs_max_len;
			rs_fetch_dir = stmt.rs_fetch_dir;
			rs_max_rows = stmt.rs_max_rows;
			rs_fetch_size = (rs_max_rows > 0  &&  rs_max_rows < preFetch) 
				? rs_max_rows : preFetch;
			tr_id = "Ftch[" + inst_id + "]";

			/*
			** Row pre-fetching must be disabled if there is a BLOB column.
			*/
			for( int col = 0; col < rsmd.count; col++ )
				if ( ProviderTypeMgr.isLong(rsmd.desc[ col ].sql_type) )
				{
					rowCacheEnabled = false;
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

//		public Statement
//			getStatement()
//		{
//			if ( trace.enabled() )  trace.log( title + ".getStatement(): " + stmt );
//			return( stmt );
//		} // getStatement


		/*
		** Name: disablePreFetch
		**
		** Description:
		**	Disables row pre-fetching and the row cache.  Rows will be
		**	fetched one at a time from the DBMS.  Partial row fetches
		**	are allowed.
		**
		**	NOTE: pre-fetching will not be disabled if pre-fetching has
		**	already occured.
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
		**	 4-Aug-03 (gordy)
		**	    Created.
		*/

		protected void
			disablePreFetch()
		{
			if ( rowCacheEnabled )
			{
				if ( rowCache == null )
					rowCacheEnabled = false;
				else  if ( trace.enabled( 1 ) )
					trace.write( tr_id + ": attempt to disable pre-fetch after init" );
			}
			return;
		} // disablePreFetch


		/*
		** Name: preLoad
		**
		** Description:
		**	Pre-load the row cache.  Returns indication that end-of-data
		**	has been reached.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	True if end-of-data reached.
		**
		** History:
		**	 6-Oct-03 (gordy)
		**	    Created.
		*/

		protected bool
			preLoad()
		{
			if ( msg.moreMessages() )	// Make sure there is something to read
			{
				initRowCache();
				readResults();
			}
    
			/*
			** If EOD was just received, then there shouldn't
			** be an active stream, but correctness dictates
			** that we check and wait to close the cursor
			** when the fetch has completed.
			*/
			return( (rslt_flags & MSG_RF_EOD) != 0  &&  activeStream == null );
		} // preLoad


		/*
		** Name: load
		**
		** Description:
		**	Position the result set to the next row and load the column
		**	values.  
		**
		**	Retrieves next row from row cache or fetches rows from DBMS.
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
		**	    Extracted multi-row handling to AdvanRslt and renamed load().
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
		*/

		protected internal override void
			load()
		{
			/*
			** Initialize row cache if needed.
			*/
			if ( rowCacheEnabled  &&  rowCache == null )  initRowCache();

			/*
			** Cleanup prior row state prior to loading next row.
			** Partial row fetches must be flushed and row-set
			** initialized to empty.
			**
			** NOTE: flush() must be called first because it relies
			** on the current row state during processing.
			*/
			if ( activeStream != null )  flush();
			column_count = 0;
	
			/*
			** Fetch row(s) from server if row cache disabled (single
			** row fetching) or empty.  Cursor may have been closed due
			** to reaching EOD on preceding request.
			*/
			if ( ! cursorClosed  &&
					(! rowCacheEnabled  ||  (rowCache.Count==0)) )
			{
				fetch();

				/* 
				** Close the cursor when end-of-data detected.
				** If EOD was just received, then there shouldn't
				** be an active stream, but correctness dictates
				** that we check and wait to close the cursor
				** when the fetch has completed.
				*/
				if ( (rslt_flags & MSG_RF_EOD) != 0  &&  activeStream == null )
					try { closeCursor(); }  catch( SqlEx ) {}
			}
    
			/*
			** Single row fetching (row cache disabled) sets the row state
			** while loading column data values and no further action is
			** required.  Row cacheing only loads the cache and leaves the
			** row state as empty.  If necessary, load a row from the cache.
			*/
			if ( rowCacheEnabled  &&  (rowCache.Count!=0) )
			{
				/*
				** The current row is moved immediatly to the free cache
				** rather than trying to do so after it has been used.
				** It can still be safely accessed while in the free cache
				** since it won't be reused until after all cached rows
				** have been used.
				*/
				columns = (SqlData[])rowCache[0];  // removeFirst
				rowCache.RemoveAt(0);
				freeCache.Add( columns );    // addLast
				column_count = rsmd.count;
	
				if ( trace.enabled( 3 ) ) 
					trace.write( tr_id + ".load(): returning cached row" );
			}

			/*
			** We are positioned on a row if at least one column is available.
			*/
			if ( column_count > 0 )
			{
				pos_status = POS_ON_ROW;
				row_status = 0;
				row_number++;
	
				/*
				** Set row status.  It is easy to determine if we are on the
				** first row, but last row detection is dependent on several
				** different factors, all of which are further dependent on
				** receiving an EOD indication on preceding fetch request.
				*/
				if ( row_number == 1 ) row_status |= ROW_IS_FIRST;
				if ( cursorClosed  &&  (! rowCacheEnabled  ||  (rowCache.Count==0)) )
					row_status |= ROW_IS_LAST;
			}
			else
			{
				/*
				** Since super-class guarantees the calling state,
				** cursor has now moved after last rows.
				*/
				pos_status = POS_AFTER;
				row_status = 0;
				row_number++;
				columns = null;
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
		**	    Extracted base functionality to AdvanRslt and renamed shut().
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

		internal override bool	// package access
			shut()
		{
			bool closing;
    
			/*
			** Local processing requires the current row/cursor
			** state which is cleared by super-class processing,
			*/
			try 
			{ 
				/*
				** Flush partial row fetches prior to closing.
				*/
				if ( activeStream != null )  flush();
				closeCursor(); 
			}
			finally 
			{ 
				if ( rowCache != null )  
				{
					rowCache.Clear();
					freeCache.Clear();
				}

				/*
				** Clear current row state and do super-class processing.
				*/
				column_count = 0;
				closing = base.shut(); 
			}
    
			return( closing );
		} // shut


		/*
		** Name: columnMap
		**
		** Description:
		**	Determines if a given column index is a part of the result
		**	set and maps the external index to the internal index.
		**
		**	Overrides super-class implementation to support partial row
		**	fetches and load referenced columns if not currently available.
		**	Calls super-class method to do basic status checks and map
		**	external/internal index values.
		**
		** Input:
		**	columnIndex	External column index [1,rsmd.count].
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int		Internal column index [0,rsmd.count - 1].
		**
		** History:
		**	 4-Aug-03 (gordy)
		**	    Extracted partial row fetch handling from AdvanRslt.
		*/

		protected override int
			columnMap( int columnIndex )
		{
			/*
			** Call super-class to do basic status checks and map index.
			*/
			columnIndex = base.columnMap( columnIndex );

			/*
			** If request is for a column of a partial row which
			** is beyond what is currently available, request that
			** data be loaded upto the current requested column.
			*/
			if ( columnIndex >= column_count )  flush( columnIndex );
			return( columnIndex );
		} // columnMap


		/*
		** Name: fetch
		**
		** Description:
		**	Called to fill row cache or read next row if pre-fetching disabled.
		**
		**	Note: Exception trapping/reporting is done by super-class.
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
		**	    Extracted multi-row handling to AdvanRslt and renamed load().
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
		**	    Renamed from load() to fetch() to allow multi-row next()
		**	    functionality to be moved into load().
		*/

		protected void
			fetch()
		{
			msg.begin( MSG_CURSOR );
			msg.write( MSG_CUR_FETCH );
			msg.write( MSG_CUR_STMT_ID );
			msg.write( (short)8 );
			msg.write( (int)((stmt_id >> 32 ) & 0xffffffff) );
			msg.write( (int)(stmt_id & 0xffffffff) );

			if ( rowCacheEnabled  &&  rs_fetch_size > 1 )
			{
				msg.write( MSG_CUR_PRE_FETCH );
				msg.write( (short)4 );
				msg.write( (int)rs_fetch_size );
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
		**	BLOB columns interrupt result processing, so this
		**	method should only be called as a result of a BLOB
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
		**	should flush all active BLOB streams).
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
		**	    Extracted base functionality to AdvanRslt and renamed shut().
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

		protected internal virtual void
			closeCursor()
		{
			/*
			** Safely check/update the cursor state.
			*/
			lock( this )
			{
				if ( cursorClosed )  return;	// Cursor already closed.
				cursorClosed = true;
			}

			/*
			** Server may have automatically closed the cursor.
			*/
			if ( (rslt_flags & MSG_RF_CLOSED) != 0 )  
			{
				if ( trace.enabled( 3 ) )
					trace.write( tr_id + ": cursor automatically closed at EOD!" );
				return;
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
				msg.write( (int)((stmt_id >> 32 ) & 0xffffffff) );
				msg.write( (int)(stmt_id & 0xffffffff) );
				msg.done( true );
				readResults();
			}
			catch( SqlEx ex )
			{
				if ( trace.enabled( 1 ) )
				{
					trace.write( tr_id + ".shut(): error closing cursor" );
					ex.trace( trace );
				}

				/*
				** Ignore error if cursor already closed in server.
				*/
				if (ex.getErrorCode() != E_GC4811_NO_STMT) throw ex;
			}

			if ( trace.enabled( 3 ) )  trace.write( tr_id + ": cursor closed!" );
			return;
		} // closeCursor


		/*
		** Name: streamClosed
		**
		** Description:
		**	Stream closure notification method for the IStreamListener
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
					trace.write( tr_id + ": invalid BLOB stream closure!" );
				return;
			}
    
			if ( trace.enabled( 4 ) )  trace.write( tr_id + ": BLOB stream closed" );

			try
			{
				/*
				** Clear stream state and continue column processing.
				*/
				activeStream = null;
				resume();
	
				/* 
				** Close the cursor when end-of-data detected.
				** If EOD was just received, then there shouldn't
				** be an active stream, but correctness dictates
				** that we check and wait to close the cursor
				** when the fetch has completed.
				*/
				if ( (rslt_flags & MSG_RF_EOD) != 0  &&  activeStream == null )
					try { closeCursor(); }  
					catch( SqlEx ) {}
			}
			catch( SqlEx ex )
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
					ex.trace( trace );
				}

				try { shut(); }  
				catch( SqlEx ) {}
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
		{
			/*
			** Query results are partially read only when a BLOB
			** column interrupts column loading due to an active
			** stream requiring processing.  Column processing
			** continues when the active stream is finished/closed.
			**
			** The easiest way to flush the input message queue is
			** close the current active stream.  This will result
			** in a stream closure notification which continues
			** column processing via the resume() method.  Column
			** processing will either complete reading the query
			** result messages or be interrupted by another BLOB
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
		**	The contents of BLOB columns preceding the requested column
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
		{
			/*
			** The only reason the requested column isn't loaded
			** is a preceding active BLOB.  Closing the stream will
			** result in a stream closure notification and column
			** processing will be resumed.  Preceding BLOB columns
			** will interrupt processing, so we loop closing active
			** streams until the requested columns has been loaded.
			*/
			while( index >= column_count  &&  activeStream != null )  
				activeStream.closeStream();
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

		protected internal override void
		setProcResult(int retVal)
		{
			if (trace.enabled(3))
				trace.write(tr_id + ".setProcResult(" + retVal + ")");
			stmt.setProcResult(retVal);
			return;
		} // setProcResult


		/*
		** Name: readData
		**
		** Description:
		**	Read data from server.  Overrides default method in DrvObj.
		**
		**	Guarantees that partial rows are only loaded when required due
		**	to BLOB column being loaded.
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
		*/

		protected internal override bool
			readData()
		{
			/*
			** Loading the row cache doesn't allow 
			** partial row loads or interrupts.
			*/
			if ( rowCacheEnabled )
			{
				loadRowCache();		// Read/cache entire rows.
				return( false );
			}

			/*
			** Reset row state if current row complete.
			*/
			if ( column_count >= rsmd.count )  column_count = 0;
			int	start = column_count;

			/*
			** Allocate the row/column data array, if needed,
			** and read column data values.
			*/
			if ( columns == null )  columns = allocateRowBuffer( rsmd );
			bool interrupt = readColumns( rsmd, columns );

			/*
			** Partial row loads should only occur due to
			** BLOB columns which interrupt processing.
			*/
			if ( column_count > 0  &&  column_count < rsmd.count  &&  ! interrupt )
			{
				if ( trace.enabled( 1 ) )  
					trace.write( tr_id + ": failed to load entire row" );
				throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
			}

			if ( trace.enabled( 3 )  &&  column_count > start )
				trace.write( tr_id + ".load(): read " + (column_count - start) + 
					" of " + rsmd.count + " columns starting with " +
					(start + 1) );

			return( interrupt );
		} // readData


		/*
		** Name: initRowCache
		**
		** Description:
		**	    Initialize the row cache.
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
		**	 6-Oct-03 (gordy)
		**	    Created.
		*/

		private void
			initRowCache()
		{
			if ( rowCacheEnabled  &&  rowCache == null )  
			{
				rowCache = new ArrayList();
				freeCache = new ArrayList();
			}
			return;
		} // initRowCache


		/*
		** Name: loadRowCache
		**
		** Description:
		**	Reads entire rows returned by the DBMS and stores them
		**	in the row cache.  Returns only when current message is
		**	empty.  Throws exception if invalid row state is detected.
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
		**	 4-Aug-03 (gordy)
		**	    Extracted from readData().
		**	22-Sep-03 (gordy)
		**	    Added method to handle row and column data value allocation.
		*/

		private void
			loadRowCache()
		{
			int row_count = 0;
    
			/*
			** We should only be called when the cache is empty,
			** but we only enforce being at the start of a row
			** (if the current row is full then we are basically
			** at the start of the next row).
			*/
			if ( column_count <= 0  ||  column_count >= rsmd.count )
				column_count = 0;	// Must be 0 for start of processing loop.
			else
			{
				if ( trace.enabled( 1 ) )
					trace.write( tr_id + ": can't load row cache due to partial row" );
				throw SqlEx.get( "DRIVER INTERNAL ERROR: invalid row cache state",
					"HY000", 0);
			}

			/*
			** A row must be contained in a single data message, but
			** the row set may be sent in multiple messages (splitting
			** only allowed between individual rows).  Loop reading 
			** rows until current message is exhausted then return to
			** allow next message to be processed.
			*/
			while( msg.moreData() )
			{
				SqlData[] row;
	
				/*
				** Allocate and load a new row of column data values.
				*/
				if ( freeCache.Count==0 )
					row = allocateRowBuffer( rsmd );
				else
				{
					row = (SqlData[])freeCache[0];  // removeFirst
					freeCache.RemoveAt(0);
				}
	
				/*
				** We expect to read an entire row.  A partial row may be
				** read if a BLOB column is found or the end of the message
				** is reached, neither of which should occur when pre-
				** fetching rows.
				*/
				if ( readColumns( rsmd, row ) )
				{
					if ( trace.enabled( 1 ) )  
						trace.write( tr_id + ": BLOB column loaded during pre-fetch" );
					throw SqlEx.get( "DRIVER INTERNAL ERROR: " +
						"BLOB columns not supported when cacheing rows",
						"HY000", 0);
				}
	
				if ( column_count < rsmd.count )
				{
					if ( trace.enabled( 1 ) )  
						trace.write( tr_id + ": pre-fetch failed to load entire row" );
					throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
				}
	
				/*
				** Save row in cache and reset for start of next row.
				*/
				rowCache.Add( row );
				row_count++;
				column_count = 0;
			}

			if ( trace.enabled( 3 ) )  trace.write( tr_id + ".load(): Loaded " + 
										   row_count + " rows into cache" );
			return;
		} // loadRowCache


		/*
		** Name: allocateRowBuffer
		**
		** Description:
		**	Allocate the column data array for a row and populate
		**	the array with a SqlData object for each column based
		**	on the column data type.
		**
		** Input:
		**	rsmd		Row-set Meta-data.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	SqlData[]	Column data array.
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
		**	 6-Jan-04 (thoda04)
		**	    Added BigInt support.
		**	16-Jun-06 (gordy)
		**	    ANSI Date/Time data type support.
		**	 7-Dec-09 (gordy, ported by thoda04)
		**	    Support BOOLEAN columns.
		*/

		private SqlData[]  allocateRowBuffer( AdvanRSMD rsmd )
		{
			SqlData[] row = new SqlData[ rsmd.count ];

			for( int col = 0; col < rsmd.count; col++ )
			{
				switch( rsmd.desc[ col ].sql_type )
				{
					case ProviderType.DBNull :	row[ col ] = new SqlNull();	break;
					case ProviderType.TinyInt :	row[ col ] = new SqlTinyInt();	break;
					case ProviderType.SmallInt:	row[ col ] = new SqlSmallInt();	break;
					case ProviderType.Integer :	row[ col ] = new SqlInt();	break;
					case ProviderType.BigInt:  	row[ col ] = new SqlBigInt();	break;
					case ProviderType.Single :	row[ col ] = new SqlReal();	break;
					case ProviderType.Double :	row[ col ] = new SqlDouble();	break;
					case ProviderType.Numeric:
					case ProviderType.Decimal :	row[ col ] = new SqlDecimal();	break;
					case ProviderType.Boolean :	row[ col ] = new SqlBool();   	break;

					case ProviderType.Date :	row[col] = new SqlDate(); break;

					case ProviderType.Time:
						row[col] = new SqlTime(rsmd.desc[col].dbms_type);
						break;

					case ProviderType.Interval:
					case ProviderType.IntervalDayToSecond:
					case ProviderType.IntervalYearToMonth:
						row[col] = new SqlInterval(rsmd.desc[col].dbms_type);
						break;

					case ProviderType.DateTime:
						switch (rsmd.desc[col].dbms_type)
						{
							case DBMS_TYPE_IDATE:
								row[col] = new IngresDate(conn.osql_dates,
												 conn.timeValuesInGMT());
								break;

							default:
								row[col] = new SqlTimestamp(rsmd.desc[col].dbms_type);
								break;
						}
						break;

					case ProviderType.Binary :
						row[ col ] = new SqlByte( rsmd.desc[ col ].length );
						break;

					case ProviderType.VarBinary :  
						row[ col ] = new SqlVarByte( rsmd.desc[ col ].length );
						break;

					case ProviderType.Char :
						if ( rsmd.desc[ col ].dbms_type != DBMS_TYPE_NCHAR )
							row[ col ] = new SqlChar( msg.getCharSet(), 
								rsmd.desc[ col ].length );
						else
							row[ col ] = new SqlNChar(rsmd.desc[col].length / 2 );
						break;

					case ProviderType.VarChar :
						if ( rsmd.desc[ col ].dbms_type != DBMS_TYPE_NVARCHAR )
							row[col] = new SqlVarChar(msg.getCharSet(), 
								rsmd.desc[col].length );
						else
							row[col] = new SqlNVarChar( rsmd.desc[col].length / 2 );
						break;

					case ProviderType.LongVarBinary :
						row[ col ] = new SqlLongByte( (SqlStream.IStreamListener)this );
						break;

					case ProviderType.LongVarChar :
						if ( rsmd.desc[ col ].dbms_type != DBMS_TYPE_LONG_NCHAR )
							row[ col ] = new SqlLongChar( msg.getCharSet(), 
								(SqlStream.IStreamListener)this );
						else
							row[ col ] = new SqlLongNChar(
								(SqlStream.IStreamListener)this );
						break;

					default :
						if ( trace.enabled( 1 ) )
							trace.write( tr_id + ": unexpected SQL type " + 
								rsmd.desc[ col ].sql_type );
						throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
				}
			}

			return( row );
		} // allocateRowBuffer


		/*
		** Name: readColumns
		**
		** Description:
		**	Read column data from server.  Assumes that the column counter, 
		**	column_count, indicates the last column read.  Reads the next
		**	column in the row and continues until message is empty , a BLOB 
		**	column is read or the last column in the row is reached.
		**
		** Input:
		**	rsmd		Row-set Meta-data
		**	row		Column data array.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean		BLOB column interrupted processing.
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
		**	    AdvanRslt now handles these as conversion errors.
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
		**	    Added BOOLEAN data type, treated same as BIT.
		**	 4-Aug-03 (gordy)
		**	    Extracted from readData() to separate row and column processing.
		**	22-Sep-03 (gordy)
		**	    Changed to use SQL Data objects to hold column values.
		**	    Extracted column data object allocation to allocateRowBuffer().
		**	 6-Jan-04 (thoda04)
		**	    Added BigInt support.
		**	16-Jun-06 (gordy)
		**	    ANSI Date/Time data type support.
		**	 7-Dec-09 (gordy, ported by thoda04)
		**	    Support BOOLEAN columns.
		*/

		private bool
			readColumns( AdvanRSMD rsmd, SqlData[] row )
		{
			/*
			** Begin processing columns after last read.
			** Processing ends after last column in row
			** is read, or if an interrupt is requested.
			** Note that the column count (1 based index)
			** references the next column to load (0 based
			** index).
			*/
			for( ; column_count < rsmd.count; column_count++ )
			{
				int col = column_count;

				/*
			** We only require individual column values, with the
			** exception of BLOB segments, to not be split across
			** messages.  If the next column is not available,
			** return without interrupt to allow caller to handle
			** the partial row.
				*/
				if ( ! msg.moreData() )  return( false );

				switch( rsmd.desc[ col ].sql_type )
				{
					case ProviderType.DBNull :
						msg.readSqlData( (SqlNull)row[ col ] );
						break;

					case ProviderType.TinyInt : 
						msg.readSqlData( (SqlTinyInt)row[ col ] );
						break;

					case ProviderType.SmallInt :	
						msg.readSqlData( (SqlSmallInt)row[ col ] );
						break;

					case ProviderType.Integer : 
						msg.readSqlData( (SqlInt)row[ col ] );
						break;

					case ProviderType.BigInt :	
						msg.readSqlData( (SqlBigInt)row[ col ] );
						break;

					case ProviderType.Single : 
						msg.readSqlData( (SqlReal)row[ col ] );
						break;

					case ProviderType.Double :
						msg.readSqlData( (SqlDouble)row[ col ] );
						break;

					case ProviderType.Decimal : 
						msg.readSqlData( (SqlDecimal)row[ col ] );
						// TODO: setWarning( new DataTruncation(col + 1, false, true, -1, 0) );
						break;

					case ProviderType.Boolean:
						msg.readSqlData((SqlBool)row[col]);
						break;

					case ProviderType.Date:
						msg.readSqlData((SqlDate)row[col]);
						break;

					case ProviderType.Time:
						msg.readSqlData((SqlTime)row[col]);
						break;

					case ProviderType.Interval:
					case ProviderType.IntervalDayToSecond:
					case ProviderType.IntervalYearToMonth:
						msg.readSqlData((SqlInterval)row[col]);
						break;

					case ProviderType.DateTime:
						switch (rsmd.desc[col].dbms_type)
						{
							case DBMS_TYPE_IDATE:
								bool isBlankDateNull =
									(conn != null)?conn.isBlankDateNull : false;
								msg.readSqlData((IngresDate)row[col], isBlankDateNull);
								break;

							default:
								msg.readSqlData((SqlTimestamp)row[col]);
								break;
						}
						break;

					case ProviderType.Binary :
						msg.readSqlData( (SqlByte)row[ col ] );
						break;

					case ProviderType.VarBinary :  
						msg.readSqlData( (SqlVarByte)row[ col ] );
						break;

					case ProviderType.Char :
						if ( rsmd.desc[ col ].dbms_type != DBMS_TYPE_NCHAR )
							msg.readSqlData( (SqlChar)row[ col ] );
						else
							msg.readSqlData( (SqlNChar)row[ col ] );
						break;

					case ProviderType.VarChar :
						if ( rsmd.desc[ col ].dbms_type != DBMS_TYPE_NVARCHAR )
							msg.readSqlData( (SqlVarChar)row[ col ] );
						else
							msg.readSqlData( (SqlNVarChar)row[ col ] );
						break;

					case ProviderType.LongVarBinary :
						/*
						** Initialize BLOB stream.
						*/
						msg.readSqlData( (SqlLongByte)row[ col ] );

						/*
						** NULL BLOBs don't require special handling.
						** Non-NULL BLOBs interrupt column processing.
						*/
						if ( ! row[ col ].isNull() )
						{
							activeStream = (SqlStream)row[ col ];	// Stream reference
							column_count++;				// Column is done
							return( true );				// Interrupt.
						}
						break;

					case ProviderType.LongVarChar :
						/*
						** Initialize BLOB stream.
						*/
						if ( rsmd.desc[ col ].dbms_type != DBMS_TYPE_LONG_NCHAR )
							msg.readSqlData( (SqlLongChar)row[ col ] );
						else
							msg.readSqlData( (SqlLongNChar)row[ col ] );

						/*
						** NULL BLOBs don't require special handling.
						** Non-NULL BLOBs interrupt column processing.
						*/
						if ( ! row[ col ].isNull() )
						{
							activeStream = (SqlStream)row[ col ];	// Stream reference
							column_count++;				// Column is done
							return( true );				// Interrupt.
						}
						break;

					default :
						if ( trace.enabled( 1 ) )
							trace.write( tr_id + ": unexpected SQL type " + 
								rsmd.desc[ col ].sql_type );
						throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
				}
			}

			return( false );
		} // readColumns


	} // class RsltFtch

}