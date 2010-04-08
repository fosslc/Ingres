/*
** Copyright (c) 1999, 2006 Ingres Corporation. All Rights Reserved.
*/

using System;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: rsltcurs.cs
	**
	** Description:
	**	Defines class which implements the ResultSet interface
	**	extending server fetching for cursor processing.
	**
	**  Classes
	**
	**	RsltCurs
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
	**	13-Dec-99 (gordy)
	**	    Added support for pre-fetching of rows.
	**	 4-May-00 (gordy)
	**	    Extracted multi-row handling to base class.
	**	27-Oct-00 (gordy)
	**	    Extracted server operational support to new base class
	**	    while keeping control of connection locking.
	**	 3-Nov-00 (gordy)
	**	    Added support for 2.0 extensions.
	**	23-Jan-01 (gordy)
	**	    Changed parameter type to AdvanStmt for backward compatibility.
	**	28-Mar-01 (gordy)
	**	    Combined functionality of close() into shut(), which is
	**	    replaced by closeCursor().
	**	31-Oct-02 (gordy)
	**	    Adapted for generic GCF driver.
	**	 4-Aug-03 (gordy)
	**	    Simplified super-class implementation removes need for
	**	    lock covers and ensures calling sequence.
	**	22-Sep-03 (gordy)
	**	    BLOB indicator replaced with stream reference.
	**	 6-Oct-03 (gordy)
	**	    Added pre-loading ability to result-sets.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	*/


	/*
	** Name: RsltCurs
	**
	** Description:
	**	driver class which implements the ResultSet interface
	**	as an extension of server row fetching by adding support for
	**	cursor processing.
	**
	**	Most of the functionality for this class is inherited from the 
	**	super-classes, especially the direct super-class RsltFtch.  The 
	**	only significant extensions involve connection locking and the
	**	cursor name.  For cursors, the connection is locked only during
	**	the cursor fetch and close requests.  The connection should be
	**	unlocked immediately after opening the cursor.
	**
	**	Data sets containing BLOBs complicate the locking scheme since 
	**	the cursor fetch must be interrupted on the driver side for BLOB 
	**	processing.  The connection must remain locked until the BLOB 
	**	has been processed and the remainder of the data set has been 
	**	received.  
	**
	**	The super-class provides three methods which may be overridden
	**	to provide connection locking: fetch(), resume(), closeCursor().  
	**	The fetch() and resume() methods are used during data loading.  
	**	The fetch() method is called only when the connection is dormant 
	**	and leaves the connection dormant unless a BLOB column is active.  
	**	The resume() methods is called when the connection is active 
	**	after an active BLOB has been flushed.  Like fetch(), resume() 
	**	leaves the connection dormant unless a BLOB column is active.  
	**	The closeCursor() method is called only when the connection is
	**	dormant and leaves the connection dormant.
	**
	**	The connection locking in the local implementations of fetch(),
	**	resume(), and closeCursor() following the guidelines listed
	**	above: locking the connection upon entry when dormant and
	**	unlocking on exit if no BLOB is active.
	**
	**  Overridden Methods:
	**
	**	getCursorName	    Retrieve the associated cursor name.
	**	fetch		    Fetch rows from DBMS.
	**	closeCursor	    Close cursor.
	**	resume		    Continue result processing after BLOB.
	**
	**  Private Data:
	**
	**	cursor		    Name of associated DBMS cursor.
	**
	** History:
	**	14-May-99 (gordy)
	**	    Created.
	**	29-Sep-99 (gordy)
	**	    Implemented support for BLOB data and added support
	**	    methods and fields.
	**	11-Nov-99 (gordy)
	**	    Extracted base functionality to AdvanRslt.
	**	 4-May-00 (gordy)
	**	    Extracted multi-row handling to base class.
	**	27-Oct-00 (gordy)
	**	    Extracted server operational support to new base class
	**	    while keeping control of connection locking.
	**	28-Mar-01 (gordy)
	**	    Renamed shut() to closeCursor().
	**	 5-Sep-02 (thoda04)
	**	    Ported for .NET Provider.
	**	 4-Aug-03 (gordy)
	**	    Simplified super-class implementation removes need for
	**	    local lock tracking: locked, lock() and unlock().  Super-
	**	    class load() renamed to fetch().  Removed flush() as it
	**	    no longer needs locking.
	*/

	class
		RsltCurs : RsltFtch
	{

		private String	cursor = null;		// Cursor name.


		/*
		** Name: RsltCurs
		**
		** Description:
		**	Class constructor.
		**
		**	Initial row cache may be pre-loaded, but only if the
		**	message stream is active and DATA messages available.
		**
		** Input:
		**	conn		Associated connection.
		**	stmt		Associated statement.
		**	rsmd		ResultSet meta-data.
		**	stmt_id		Statement ID.
		**	cursor		Cursor name.
		**	preFetch	Pre-fetch row count.
		**	preLoad		Load initial row cache.
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
		**	27-Oct-00 (gordy)
		**	    Pass statement ID to new super class constructor.
		**	 3-Nov-00 (gordy)
		**	    Changed parameters for 2.0 extensions.
		**	23-Jan-01 (gordy)
		**	    Changed parameter type to AdvanStmt for backward compatibility.
		**	31-Oct-02 (gordy)
		**	    Adapted for generic GCF driver.
		**	 6-Oct-03 (gordy)
		**	    Added preLoad parameter to read initial row cache from server.
		*/

		public
			RsltCurs
			( 
			DrvConn	conn,
			AdvanStmt	stmt, 
			AdvanRSMD	rsmd, 
			long	stmt_id,
			String	cursor,
			int		preFetch,
			bool	do_preLoad ) : base( conn, stmt, rsmd, stmt_id, preFetch )

		{
			this.cursor = cursor;
			tr_id = "Curs[" + inst_id + "]";
    
			/*
			** Pre-load row cache if requested.
			*/
			if ( do_preLoad  &&  preLoad() )  
			{
				/* 
				** Close the statement when end-of-data detected.
				**
				** Note that we don't call our closeCursor() method
				** since it expects to handle the connection lock
				** state.  Our caller is handling the lock state
				** during initialization.
				*/
				try { base.closeCursor(); }  
				catch( SqlEx ) {}
			}
			return;
		} // RsltCurs


		/*
		** Name: getCursorName
		**
		** Description:
		**	Return the cursor name associated with the result set.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	    Cursor name.
		**
		** History:
		**	14-May-99 (gordy)
		**	    Created.
		*/

		public new String
			getCursorName()
		{
			if ( trace.enabled() )  trace.log( title + ".getCursorName(): " + cursor );
			return( cursor );
		} // getCursorName


		/*
		** Name: fetch
		**
		** Description:
		**	Fetch rows from the server.  Adds locking to super-class method.
		**
		**	Locks the connection prior to calling super-class method.  
		**	Connection is left locked if a BLOB is active (column loading
		**	was interrupted).  Otherwise, the connection is unlocked.
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
		**	27-Oct-00 (gordy)
		**	    Created.
		**	 4-Aug-03 (gordy)
		**	    Renamed by super-class.  Simplified super-class implementation 
		**	    removes need for lock covers and ensures calling sequence.
		**	22-Sep-03 (gordy)
		**	    BLOB indicator replaced with stream reference.
		*/

		protected new void
			fetch()

		{
			msg.LockConnection();
			try
			{
				base.fetch();
				if ( activeStream == null )  msg.UnlockConnection();
			}
			catch( SqlEx ex )
			{
				msg.UnlockConnection();
				throw ex;
			}
			return;
		}


		/*
		** Name: resume
		**
		** Description:
		**	Continue processing query results following column which
		**	interrupted processing.  Adds locking to super-class method
		**
		**	Connection is only left locked when a BLOB is active.  Any
		**	action on the connection from this result-set flushes the
		**	active BLOB and resumes loading column values.  Connection
		**	is locked when called and is unlocked if no BLOB is active
		**	following processing.
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
		**	27-Oct-00 (gordy)
		**	    Created.
		**	31-Oct-02 (gordy)
		**	    Connection should be locked, but since lock() handles
		**	    such a case, added a call just to be sure.
		**	 4-Aug-03 (gordy)
		**	    Simplified super-class implementation removes need for
		**	    lock covers and ensures calling sequence.
		**	22-Sep-03 (gordy)
		**	    BLOB indicator replaced with stream reference.
		*/

		protected new void
			resume()
		{
			try 
			{ 
				base.resume(); 
				if ( activeStream == null )  msg.UnlockConnection();
			}
			catch( SqlEx ex )
			{
				msg.UnlockConnection();
				throw ex;
			}
			return;
		} // resume


		/*
		** Name: closeCursor
		**
		** Description:
		**	Close the cursor and free associated resources.  Adds locking 
		**	to super-class method.
		**
		**	Locks the connection prior to calling super class-method.  
		**	Connection is unlocked after closing.
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
		**	27-Oct-00 (gordy)
		**	    Created.
		**	28-Mar-01 (gordy)
		**	    Renamed to closeCursor().
		**	 4-Aug-03 (gordy)
		**	    Simplified super-class implementation removes need for
		**	    lock covers and ensures calling sequence.
		**	 6-Oct-03 (gordy)
		**	    Don't need to lock() connection when cursor already closed.
		*/

		protected internal override void
			closeCursor()
		{
			/*
			** There is no need to lock the connection if
			** already closed since there is nothing to do.
			*/
			if ( ! cursorClosed )
			{
				msg.LockConnection();
				try { base.closeCursor(); }
				finally { msg.UnlockConnection(); }
			}
			return;
		} // closeCursor


	} // class RsltCurs

}