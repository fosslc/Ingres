/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbc;

/*
** Name: RsltSlct.java
**
** Description:
**	Implements the EDBC JDBC ResultSet class RsltSlct
**	for handling select-loop result-sets.
**
**  Classes
**
**	RsltSlct	Select-loop result-set.
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
*/

import	java.sql.SQLWarning;
import	ca.edbc.util.EdbcEx;
import	ca.edbc.io.DbConn;


/*
** Name: RsltSlct
**
** Description:
**	EDBC JDBC Java driver class which implements the
**	JDBC ResultSet interface for DBMS result data which
**	is obtained through a select-loop.
**
**	Most of the functionality for this class is inherited
**	from the super-classes, especially the direct super-
**	class RsltFtch.  The only significant extension for
**	a select-loop result-set is connection locking.  For 
**	select-loops, the connection is locked from the start 
**	of the query until the result set is closed.  
**
**	Although select loops produce a continuous data stream
**	from the DBMS server, result-set data is still handled 
**	on a fetch request basis between the driver and the JDBC 
**	server.  The JDBC server hides the difference between
**	cursor fetches and select-loop tuple streams.
**
**  Overridden Methods:
**
**	closeCursor	    Close cursor.
**
** History:
**	17-May-00 (gordy)
**	    Created.
**	27-Oct-00 (gordy)
**	    New super class which doesn't enforce connection
**	    locking or cursor oriented operations.
**	28-Mar-01 (gordy)
**	    Rename shut() to closeCursor().
*/

class
RsltSlct
    extends RsltFtch
{

/*
** Name: RsltSlct
**
** Description:
**	Class constructor.
**
** Input:
**	stmt		Associated statement.
**	rsmd		Result set Meta-Data.
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
*/

public
RsltSlct
( 
    EdbcStmt	stmt, 
    EdbcRSMD	rsmd, 
    long	stmt_id,
    int		preFetch
)
    throws EdbcEx
{
    super( stmt, rsmd, stmt_id, preFetch );
    tr_id = "Slct[" + inst_id + "]";
} // RsltSlct


/*
** Name: closeCursor
**
** Description:
**	Close the select loop.  The connection is unlocked when
**	the select-loop has been closed.
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
*/

protected void
closeCursor()
    throws EdbcEx
{
    if ( cursorClosed )  return;

    try { super.closeCursor(); }
    catch( EdbcEx ex )
    {
	if ( ex.getErrorCode() != E_AP0009_QUERY_CANCELLED.code )
	    throw ex;
	else
	{
	    /*
	    ** Closing the select loop may require cancelling
	    ** the query.  Its not really an error, but the
	    ** info may be useful to the client, so translate
	    ** into a warning message.
	    */
	    setWarning( EdbcEx.getWarning( ex ) );
	    if ( (ex = (EdbcEx)ex.getNextException()) != null )  throw ex;
	}
    }
    finally { dbc.unlock(); }

    return;
} // closeCursor

} // class RsltSlct

