/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbc;

/*
** Name: RsltCurs.java
**
** Description:
**	Implements the EDBC JDBC ResultSet class RsltCurs
**	for handling cursor result-sets.  Extends RsltFtch
**	class by adding dbc locking and support for a cursor
**	name.
**
**  Classes
**
**	RsltCurs	Cursor result-set.
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
**	13-Dec-99 (gordy)
**	    Added support for pre-fetching of rows.
**	 4-May-00 (gordy)
**	    Extracted multi-row handling to base class.
**	27-Oct-00 (gordy)
**	    Extracted server operational support to new base class
**	    while keeping control of connection locking.
**	 3-Nov-00 (gordy)
**	    Added support for JDBC 2.0 extensions.
**	23-Jan-01 (gordy)
**	    Changed parameter type to EdbcStmt for backward compatibility.
**	28-Mar-01 (gordy)
**	    Combined functionality of close() into shut(), which is
**	    replaced by closeCursor().
*/

import	java.sql.SQLException;
import	ca.edbc.util.EdbcEx;
import	ca.edbc.io.DbConn;

/*
** Name: RsltCurs
**
** Description:
**	EDBC JDBC Java driver class which implements the
**	JDBC ResultSet interface for DBMS result data which
**	is obtained through a cursor.
**
**	Most of the functionality for this class is inherited
**	from the super-classes, especially the direct super-
**	class RsltFtch.  The only significant extensions for
**	a cursor result-set are connection locking and a cursor
**	name.  For cursors, the connection is locked only during
**	the cursor fetch and close requests. 
**
**	Data sets containing BLOBs complicate the locking scheme
**	since the cursor fetch must be interrupted on the driver
**	side for BLOB processing.  The connection must remain
**	locked until the BLOB has been processed and the remainder
**	of the data set has been received.  The interaction of 
**	the load(), shut(), flush() and resume() methods makes it 
**	difficult to know the precise data set processing state at 
**	this level.  For this reason, this result-set maintains 
**	its own locking state so that locking requests are only 
**	performed when actually needed.
**
**  Overridden Methods:
**
**	load		    Load data subset.
**	closeCursor	    Close cursor.
**	flush		    Flush the input message stream.
**	resume		    Continue result processing after BLOB.
**	getCursorName	    Retrieve the associated cursor name.
**
**
**  Private Methods:
**
**	lock		    Lock the connection.
**	unlock		    Unlock the connection.
**
**  Local Data:
**
**	cursor		    Name of cursor associated with the result set.
**	locked		    Lock state.
**
** History:
**	14-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented support for BLOB data and added support
**	    methods and fields.
**	11-Nov-99 (gordy)
**	    Extracted base functionality to EdbcRslt.
**	 4-May-00 (gordy)
**	    Extracted multi-row handling to base class.
**	27-Oct-00 (gordy)
**	    Extracted server operational support to new base class
**	    while keeping control of connection locking.
*/

class
RsltCurs
    extends RsltFtch
{

    private String	cursor = null;		// Cursor name.
    private boolean	locked = false;		// Lock state.


/*
** Name: RsltCurs
**
** Description:
**	Class constructor.
**
** Input:
**	stmt		Associated statement.
**	rsmd		Result set Meta-Data.
**	stmt_id		Statement ID.
**	cursor		Cursor name.
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
**	27-Oct-00 (gordy)
**	    Pass statement ID to new super class constructor.
**	 3-Nov-00 (gordy)
**	    Changed parameters for JDBC 2.0 extensions.
**	23-Jan-01 (gordy)
**	    Changed parameter type to EdbcStmt for backward compatibility.
**	28-Mar-01 (gordy)
**	    Renamed shut() to closeCursor().
*/

public
RsltCurs
( 
    EdbcStmt	stmt, 
    EdbcRSMD	rsmd, 
    long	stmt_id,
    String	cursor,
    int		preFetch
)
    throws EdbcEx
{
    super( stmt, rsmd, stmt_id, preFetch );
    this.cursor = cursor;
    tr_id = "Curs[" + inst_id + "]";
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

public String
getCursorName()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getCursorName(): " + cursor );
    return( cursor );
} // getCursorName


/*
** Name: load
**
** Description:
**	Load a new data subset.  Adds locking to super class method.
**
**	Locks the connection prior to calling super class method (lock
**	may already exist due to active BLOB).  Connection is left
**	locked if a BLOB is active following the load,  Otherwise, 
**	the connection is unlocked.
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
*/

protected void
load()
    throws EdbcEx
{
    lock();
    try
    {
	super.load();
	if ( ! blob_active )  unlock();
    }
    catch( EdbcEx ex )
    {
	unlock();
	throw ex;
    }

    return;
}


/*
** Name: closeCursor
**
** Description:
**	Close the cursor and free associated resources.  Adds locking 
**	to super class method.
**
**	Locks the connection prior to calling super class method (lock
**	may already exist due to active BLOB).  Connection is unlocked
**	after closing.
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
*/

protected void
closeCursor()
    throws EdbcEx
{
    lock();
    try { super.closeCursor(); }
    finally { unlock(); }
    return;
} // closeCursor


/*
** Name: flush
**
** Description:
**	Flush the input message stream.  Adds locking to super class method.
**
**	This routine is called while the connection is locked and leaves
**	the connection locked upon return.  The connection is locked here
**	if an active BLOB is flushed (connection unlocked in resume()).
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
*/

protected void
flush()
    throws EdbcEx
{
    try { super.flush(); }
    finally { lock(); }
    return;
}


/*
** Name: resume
**
** Description:
**	Continue processing query results following column which
**	interrupted processing.  Adds locking to super class method
**
**	Only called if BLOB is being closed so connection should be
**	locked.  The connection is unlocked if no BLOB is active
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
*/

protected void
resume()
    throws EdbcEx
{
    try 
    { 
	super.resume(); 
	if ( ! blob_active )  unlock();
    }
    catch( EdbcEx ex )
    {
	unlock();
	throw ex;
    }

    return;
} // resume


/*
** Name: lock
**
** Description:
**	Lock the DB connection.  This is just a cover for 
**	dbc.lock() with the addition of tracking the lock
**	state established by this result-set.
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
**	17-May-00 (gordy)
**	    Created.
**	27-Oct-00 (gordy)
**	    Added lock state.
*/

private void
lock()
    throws EdbcEx
{
    if ( ! locked )
    {
	dbc.lock();
	locked = true;
    }
    return;
}


/*
** Name: unlock
**
** Description:
**	Unlock the DB connection.  This is just a cover for
**	dbc.unlock() with the addition of tracking the lock
**	state established by this result-set.
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
**	17-May-00 (gordy)
**	    Created.
**	27-Oct-00 (gordy)
**	    Added lock state.
*/

private void
unlock()
{
    if ( locked )
    {
	locked = false;
	dbc.unlock();
    }
    return;
}


} // class RsltCurs

