/*
** Copyright (c) 2010 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: RsltScroll.java
**
** Description:
**	Defines class which implements the JDBC ResultSet interface
**	and supports scrollable cursors.
**
**  Classes
**
**	RsltScroll
**
** History:
**	 6-Apr-07 (gordy)
**	    Created.
**	20-Jul-07 (gordy)
**	    Row class now encapsulates a ResultSet row.
**      05-Jan-09 (rajus01) SIR 121238
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to
**            support JDBC 4.0 SQLException hierarchy.
**	 9-Aug-10 (gordy)
**	    For an empty result-set, isBeforeFirst() is supposed to return
**	    false.  It cannot be determined if the result-set is empty if
**	    no rows have been fetched.  Implement isBeforeFirst() to detect
**	    this condition and determine if the result-set is empty.  Also,
**	    isLast() cannot always determine if the highest row fetched is
**	    also the last.  Implement isLast() to detect this condition
**	    and determine if there are any more rows in the result set.
*/

import	java.sql.ResultSet;
import	java.sql.SQLException;
import	com.ingres.gcf.util.SqlExFactory;


/*
** Name: RsltScroll
**
** Description:
**	JDBC driver class which implements the JDBC ResultSet interface
**	and adds support for scrollable cursors.
**
**	Overriding the positioned load() method utilizes scrolling
**	support built into the super classes.
**
**  Overridden Methods:
**
**	load			Load next row-set.
**
** History:
**	 6-Apr-07 (gordy)
**	    Created.
**	20-Jul-07 (gordy)
**	    Super-class now tracks number of rows in result-set
**	    and provides most of the capabilities to do scrolling.
**	 9-Aug-10 (gordy)
**	    Implement isBeforeFirst() and isLast.
*/

public class
RsltScroll
    extends RsltCurs
{


/*
** Name: RsltScroll
**
** Description:
**	Class constructor.
**
**	Current implementation enforces single row per fetch.
**	Pre-fetching blocks of rows is an intended future
**	enhancement, so pre-fetch size and pre-load request
**	are accepted as parameters.
**
** Input:
**	conn		Associated connection.
**	stmt		Associated statement.
**	rsmd		ResultSet meta-data.
**	stmt_id		Statement ID.
**	cursor		Cursor name.
**	preFetch	Number of rows to pre-fetch.
**	preLoad		Load initial row cache.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 6-Apr-07 (gordy)
**	    Created.
**	20-Jul-07 (gordy)
**	    Pre-fetching (and pre-loading) now supported.
*/

RsltScroll
( 
    DrvConn	conn,
    JdbcStmt	stmt, 
    JdbcRSMD	rsmd, 
    long	stmt_id,
    String	cursor,
    int		preFetch,
    boolean	preLoad
)
    throws SQLException
{
    /*
    ** Delay pre-load until fully initialized.
    */
    super( conn, stmt, rsmd, stmt_id, cursor, preFetch, false );

    rs_type = ResultSet.TYPE_SCROLL_INSENSITIVE;
    tr_id = "Scroll[" + inst_id + "]";
    if ( preLoad )  preLoad();

} // RsltScroll


/*
** Name: load
**
** Description:
**	Position the result set to a specific row and optionally load 
**	the column values.  
**
** Input:
**	reference	Reference position.
**	offset		Row offset from reference.
**	load		TRUE to load row, FALSE to just position.
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
**	    Super-class now provides loading for scrolling along
**	    with pre-fetch cacheing of rows.
*/

protected void
load( int reference, int offset, boolean load )
    throws SQLException
{
    if ( trace.enabled( 3 ) )  trace.write( tr_id + ".load(" + reference + 
					    "," + offset + "," + load + ")" );

    /*
    ** Pre-fetching is only done for NEXT operations.
    */
    int rows = (reference == ROW  &&  offset == 1) ? fetchSize() : 1;

    try { load( reference, offset, load ? rows : 0 ); }
    catch( SQLException ex )
    {
	if ( trace.enabled() )
	    trace.log( title + ": error positioning/loading row" );
	if ( trace.enabled( 1 ) )  SqlExFactory.trace( ex, trace );
	try { shut(); }  catch( SQLException ignore ) {}
	throw ex;
    }

    return;
} // load


/*
** Name: isBeforeFirst
**
** Description:
**	Is cursor before first row of the result-set.
**
**	An empty result-set cannot be detected if a fetch request has
**	not been made.  If this is the case, prefetch the first block 
**	of rows to determine if the result-set is empty.
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
**	 9-Aug-10 (gordy)
**	    Created.
*/

public boolean
isBeforeFirst()
    throws SQLException
{
    /*
    ** An empty result-set is indicated by a rowCount of 0.  If
    ** a result-set size has not been determined (rowCount < 0),
    ** the highest row fetched can be used to detect a non-empty
    ** result-set.  The super class implementation only returns
    ** the wrong value when the result-set is empty and no rows
    ** have been fetched.  Check for this specific condition.
    */
    if ( posStatus == BEFORE  &&  rowCount < 0  &&  maxRow < 1 )
    {
	if ( trace.enabled() )  trace.log( title + ".isBeforeFirst()" );

	flush();
	warnings = null;

	try
	{
	    /*
	    ** Load the first block of rows.  If the result-set is
	    ** empty, then the result-set size will be known.  If
	    ** the result-set is not empty, then the first block of
	    ** rows will be in the cache and available for subsequent
	    ** operations.
	    */
	    load( ROW, 1, fetchSize() );

	    /*
	    ** Reposition the logical cursor back to before the first row.
	    */
	    flush();
	    warnings = null;
	    load( BEFORE, 0, 0 );
	}
	catch( SQLException ex )
	{
	    if ( trace.enabled() )
		trace.log( title + ": error positioning/loading row" );
	    if ( trace.enabled( 1 ) )  SqlExFactory.trace( ex, trace );
	    try { shut(); }  catch( SQLException ignore ) {}
	    throw ex;
	}
    }

    /*
    ** The result-set info is now sufficient for the super class
    ** implementation to return the correct result.
    */
    return( super.isBeforeFirst() );
}


/*
** Name: isLast
**
** Description:
**	Is cursor on last row of the result-set.
**
**	If the physical cursor has not been moved beyond the current
**	row, it may not be known whether it is the last row or not.
**	A fetch is required if a row cannot positively identified as
**	last.
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
**	 9-Aug-10 (gordy)
**	    Created.
*/

public boolean
isLast()
    throws SQLException
{
    /*
    ** If the current row is the highest row fetched but is not
    ** marked as LAST, then a fetch is required to determine if
    ** this really is the last row or if more rows follow.
    */
    if ( posStatus == ROW  &&  currentRow.id >= maxRow  &&
	 (currentRow.status & Row.LAST) == 0 )
    {
	if ( trace.enabled() )  trace.log( title + ".isLast()" );

	flush();
	warnings = null;

	try
	{
	    /*
	    ** The easiest thing to do is load a block of rows starting
	    ** with the current row.  This will determine if it is the
	    ** last row, and if not will pre-fetch the next block of 
	    ** rows for processing.  This will only work if the pre-
	    ** fetch block size is more than one row.
	    */
	    int size = fetchSize();

	    if ( size > 1 )
		load( ROW, 0, size );
	    else
	    {
		/*
		** With fetches restricted to one row, the next row
		** row must be fetched.  This will determine if there
		** is another row or the end of the result-set has
		** been reached.  In either case, the cursor must be
		** moved back to the original row.
		*/
		load( ROW, 1, size );

		flush();
		warnings = null;
		load( ROW, -1, size );
	    }
	}
	catch( SQLException ex )
	{
	    if ( trace.enabled() )
		trace.log( title + ": error positioning/loading row" );
	    if ( trace.enabled( 1 ) )  SqlExFactory.trace( ex, trace );
	    try { shut(); }  catch( SQLException ignore ) {}
	    throw ex;
	}
    }

    /*
    ** The result-set info is now sufficient for the super class
    ** implementation to return the correct result.
    */
    return( super.isLast() );
} // isLast


} // class RsltScroll
