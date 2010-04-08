/*
** Copyright (c) 1999, 2007 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: RsltSlct.java
**
** Description:
**	Defines class which implements the JDBC ResultSet interface
**	extending server fetching for select-loop processing.
**
**  Classes
**
**	RsltSlct
**
** History:
**	17-May-00 (gordy)
**	    Created.
**	 4-Oct-00 (gordy)
**	    Standardized internal tracing.
**	27-Oct-00 (gordy)
**	    New super class which doesn't enforce connection
**	    locking or cursor oriented operations.
**	15-Nov-00 (gordy)
**	    Added support for JDBC 2.0 extensions.
**	23-Jan-01 (gordy)
**	    Changed parameter type to EdbcStmt for backward compatibility.
**	31-Jan-01 (gordy)
**	    Use methods to handle SQLWarning.
**	28-Mar-01 (gordy)
**	    Combined functionality of close() into shut(), which is
**	    replaced by closeCursor().
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 6-Oct-03 (gordy)
**	    Added pre-loading ability to result-sets.
**	11-Oct-05 (gordy)
**	    Handle errors during data pre-load by closing the cursor.
**	 4-May-07 (gordy)
**	    Set class access for reflection.
**	20-Jul-07 (gordy)
**	    Row class now encapsulates a ResultSet row.
**      05-Jan-09 (rajus01) SIR 121238
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to
**            support JDBC 4.0 SQLException hierarchy.
*/

import	java.sql.SQLWarning;
import	java.sql.SQLException;
import	com.ingres.gcf.dam.MsgConst;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.util.SqlWarn;


/*
** Name: RsltSlct
**
** Description:
**	JDBC driver class which implements the JDBC ResultSet interface
**	as an extension of server row fetching by adding support for
**	select-loop processing.
**
**	Most of the functionality for this class is inherited from the 
**	super-classes, especially the direct super-class RsltFtch.  The 
**	only significant extension involves connection locking.  For 
**	select-loops, the connection is locked from the start of the 
**	query until the result set is closed.  The connection should
**	not be unlocked after issuing the SELECT query.
**
**	Although select loops produce a continuous data stream from the 
**	DBMS server, result-set data is still handled on a fetch request 
**	basis between the driver and server.  The server hides the 
**	difference between cursor fetches and select-loop tuple streams.
**
**  Overridden Methods:
**
**	closeCursor		Close simulated cursor.
**
**  Private Data:
**
**	locked			Connection is locked?
**
** History:
**	17-May-00 (gordy)
**	    Created.
**	27-Oct-00 (gordy)
**	    New super class which doesn't enforce connection
**	    locking or cursor oriented operations.
**	28-Mar-01 (gordy)
**	    Rename shut() to closeCursor().
**	 4-May-07 (gordy)
**	    Class is exposed outside package, permit access.
**	20-Jul-07 (gordy)
**	    Need to track connection lock state because super-class
**	    now handles EOD and auto close in more generic way.
**	    Added constructor to replace functionality of byref sub-
**	    class: loads a single initial row.
*/

public class
RsltSlct
    extends RsltFtch
    implements MsgConst
{

    private boolean		locked = true;	// Connection initially locked.


/*
** Name: RsltSlct
**
** Description:
**	Class constructor.
**
**	Pre-fetch row count may be negative if no pre-fetching
**	is permitted and zero if there is no suggested count.
**
**	Initial row cache may be pre-loaded, but only if the
**	message stream is active and DATA messages available.
**
** Input:
**	conn		Associated connection.
**	stmt		Associated statement.
**	rsmd		ResultSet meta-data.
**	stmt_id		Statement ID.
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
**	17-May-00 (gordy)
**	    Created.
**	 4-Oct-00 (gordy)
**	    Create unique ID for standardized internal tracing.
**	27-Oct-00 (gordy)
**	    New super-class constructor doesn't take cursor name.
**	15-Nov-00 (gordy)
**	    Changed parameters for JDBC 2.0 support.
**	23-Jan-01 (gordy)
**	    Changed parameter type to EdbcStmt for backward compatibility.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 6-Oct-03 (gordy)
**	    Added preLoad parameter to read initial row cache from server.
**	11-Oct-05 (gordy)
**	    Handle errors during data pre-load by closing the cursor.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, restrict constructor access.
**	20-Jul-07 (gordy)
**	    Super-class constructor takes max & pre-fetch sizes.  
**	    Super-class now handles EOD and auto-close conditions.
*/

// package access
RsltSlct
( 
    DrvConn	conn,
    JdbcStmt	stmt, 
    JdbcRSMD	rsmd, 
    long	stmt_id,
    int		preFetch,
    boolean	preLoad
)
    throws SQLException
{
    super( conn, stmt, rsmd, stmt_id, (preFetch < 0) ? 1 : 0,
				      (preFetch > 0) ? preFetch : 0 );

    tr_id = "Slct[" + inst_id + "]";
    if ( ! preLoad )  return;
    
    /*
    ** Pre-load row cache.  Statement must be closed if an error 
    ** occurs since caller won't have a result-set to close.
    **
    ** Note that our caller only locks the connection, and expects 
    ** the connection to be unlocked when data stream is done. Our 
    ** closeCursor() method will unlock the connection as needed.
    */
    try { preLoad(); }
    catch( SQLException ex )
    {
	/*
	** Close the statement and accumulate exceptions.
	*/
	try { closeCursor(); }  
	catch( SQLException ex1 ) { ex.setNextException( ex1 ); }
	throw ex;
    }

    return;
} // RsltSlct


/*
** Name: RsltSlct
**
** Description:
**	Class constructor.  Loads the initial row-set and first row.
**	In general, this constructor should only be used when one,
**	and only one, row is expected.
**
**	Initial row cache may be pre-loaded, but only if the
**	message stream is active and DATA messages available.
**
** Input:
**	conn		Associated connection.
**	stmt		Associated statement.
**	rsmd		ResultSet meta-data.
**	stmt_id		Statement ID.
**	preLoad		Load initial row cache.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	20-Jul-07 (gordy)
**	    Created.
*/

// package access
RsltSlct
(
    DrvConn	conn,
    JdbcStmt	stmt,
    JdbcRSMD	rsmd,
    long	stmt_id,
    boolean	preLoad
)
    throws SQLException
{
    this( conn, stmt, rsmd, stmt_id, -1, preLoad );

    /*
    ** Load the first (only) row and close the statement 
    ** to unlock the connection.  A row is expected.
    */
    try
    {
	load();
	try { closeCursor(); } catch( SQLException ignore ) {}
	if ( posStatus != ROW )  throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  trace.log( title + ": error loading result" );
	if ( trace.enabled( 1 ) )  SqlExFactory.trace( ex, trace );
	try { shut(); } catch( SQLException ignore ) {}
	throw ex;
    }
} // RsltSlct


/*
** Name: closeCursor
**
** Description:
**	Close the select loop.  The super-class implementation of
**	this method is used to perform the close.  The connection 
**	is unlocked when the select-loop has been closed.
**
**	Closing the select loop may require cancelling the query.  
**	Its not really an error, but the info may be useful to the 
**	client, so the error is translated into a warning message.
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
**	31-Jan-01 (gordy)
**	    Use methods to handle SQLWarning.
**	28-Mar-01 (gordy)
**	    Renamed to closeCursor().  Field closed renamed to cursorClosed.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	20-Jul-07 (gordy)
**	    May need to unlock connection when statement automatically closed.
*/

protected void
closeCursor()
    throws SQLException
{
    try { super.closeCursor(); }
    catch( SQLException ex )
    {
	/*
	** Translate query cancellation error to a warning.
	*/
	if ( ex.getErrorCode() == E_AP0009_QUERY_CANCELLED )
	{
	    setWarning( SqlWarn.get( ex ) );
	    ex = ex.getNextException();	// Check for other errors.
	}

	if ( ex != null )  throw ex;	// Original or next exception
    }
    finally 
    { 
	/*
	** If the statement was automatically closed at EOD,
	** we still may need to unlock the connection.
	*/
	synchronized( this )
	{
	    if ( locked )
	    {
		locked = false;
		msg.unlock();	// Unlock the connection.
	    }
	}
    }

    return;
} // closeCursor


} // class RsltSlct


