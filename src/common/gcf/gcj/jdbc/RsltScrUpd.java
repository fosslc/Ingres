/*
** Copyright (c) 2007 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: RsltScrUpd.java
**
** Description:
**	Defines class which implements the JDBC ResultSet interface
**	and supports updatable scrollable cursors.
**
**  Classes
**
**	RsltScrUpd
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
*/

import	java.sql.ResultSet;
import	java.sql.SQLException;
import	com.ingres.gcf.util.SqlExFactory;


/*
** Name: RsltScrUpd
**
** Description:
**	JDBC driver class which implements the JDBC ResultSet interface
**	and adds support for updatable scrollable cursors.
**
**	Overriding the positioned load() method utilizes scrolling
**	support built into the super classes.  The resume() method
**	is also overridden so that row status information available
**	at the end of the tuple block can be loaded.
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
*/

public class
RsltScrUpd
    extends RsltUpd
{


/*
** Name: RsltScrUpd
**
** Description:
**	Class constructor.
**
** Input:
**	conn		Associated connection.
**	stmt		Associated statement.
**	rsmd		ResultSet meta-data.
**	stmt_id		Statement ID.
**	cursor		Cursor name.
**	table		Updatable table name.
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
*/

RsltScrUpd
( 
    DrvConn	conn,
    JdbcStmt	stmt, 
    JdbcRSMD	rsmd, 
    long	stmt_id,
    String	cursor,
    String	table
)
    throws SQLException
{
    super( conn, stmt, rsmd, stmt_id, cursor, table );
    rs_type = ResultSet.TYPE_SCROLL_SENSITIVE;
    tr_id = "ScrUpd[" + inst_id + "]";
    return;
} // RsltScrUpd


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
*/

protected void
load( int reference, int offset, boolean load )
    throws SQLException
{
    if ( trace.enabled( 3 ) )  trace.write( tr_id + ".load(" + reference +
					    "," + offset + "," + load + ")" );

    if ( insert )
    {
	if ( trace.enabled(1) )  trace.write( tr_id + ".load: invalid row" );
	throw SqlExFactory.get( ERR_GC4021_INVALID_ROW ); 
    }
    
    params.clear( false );
	
    try { load( reference, offset, load ? 1 : 0 ); }
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


} // class RsltScrUpd
