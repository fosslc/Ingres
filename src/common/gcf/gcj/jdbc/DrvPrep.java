/*
** Copyright (c) 2004 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: DrvPrep.java
**
** Description:
**	Defines class which represents a DBMS prepared statement.
**
**  Classes:
**
**	DrvPrep
**
** History:
**	 5-May-04 (gordy)
**	    Created.
**	 9-Nov-06 (gordy)
**	    Handle query text over 64K.
**	10-Nov-06 (gordy)
**	    Changes to permit easier management in statement pool.
**	    Added use count and associated methods.
**      24-Nov-08 (rajus01) SIR 121238
**          Replaced SqlEx references with SQLException or SqlExFactory
**          depending upon the usage of it. SqlEx becomes obsolete to support
**          JDBC 4.0 SQLException hierarchy.
*/

import	com.ingres.gcf.util.SqlExFactory;
import	java.sql.SQLException;

/*
** Name: DrvPrep
**
** Description:
**	JDBC driver class which represents a DBMS prepared statement.
**
**	The query text for the statement is parsed and prepared during
**	construction only.  To optimize statement preparation, only one
**	object per unique query should be created during a transaction.
**      References to prepared statement objects should be dropped
**	when transactions end (DBMS prepared statement is dropped).
**
**  Public Methods:
**
**	prepare			Prepare statement.
**	getMetaData		Retrieve the ResultSet meta-data.
**	getStmtName		Retrieve associated statement name.
**	getTableName		Retrieve SELECT target table name.
**	getConcurrency		Retrieve statement concurrency.
**	attach			Activate association with statement.
**	detach			De-activate association with statement.
**	isActive		Does statement have active association.
**
**  Private Data:
**
**	query			Query text.
**	rsmd			ResultSet meta-data for statement.
**	stmtName		Statement name.
**	tableName		Updatable table name.
**	concurrency		Query concurrency.
**
** History:
**	 5-May-04 (gordy)
**	    Created.
**	10-Nov-06 (gordy)
**	    Separated statement preparation into method prepare().
**	    Removed statement name prefix.  Added refCount and
**	    attach(), detach(), isActive() methods.
*/

class
DrvPrep
    extends	DrvObj
{

    private int			refCount = 0;	// Active association count
    private String		query = null;
    private JdbcRSMD		rsmd = null;
    private String		stmtName = null;
    private String		tableName = null;
    private int			concurrency = DRV_CRSR_DBMS;

    
/*
** Name: DrvPrep
**
** Description:
**	Class constructor.  
**
**	Process query text.  The statment is not prepared.
**
** Input:
**	conn	    Associated connection.
**	query	    Query text.
**	stmtName    Statement name.
**
** Output:
**	None.
**
** History:
**	 5-May-04 (gordy)
**	    Created.
**	19-Jun-06 (gordy)
**	    I give up... GMT indicator replaced with connection.
**	 9-Nov-06 (gordy)
**	    Added writeQueryText() to segment query text.
**	10-Nov-06 (gordy)
**	    Statement name is now passed in rather than generated.
*/

public
DrvPrep( DrvConn conn, String query, String stmtName )
    throws SQLException
{
    super( conn );
    title = trace.getTraceName() + "-DrvPrep[" + inst_id + "]";
    tr_id = "DrvPrep[" + inst_id + "]";
    
    /*
    ** Process the query text and extract necessary info.
    ** Assign a unique statement name.
    */
    try
    {
	SqlParse sql = new SqlParse( query, conn );
	this.query = sql.parseSQL( true );
	concurrency = sql.getConcurrency();
	tableName = sql.getTableName();
    }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": error parsing statement text" );
	throw ex;
    }

    this.stmtName = stmtName;
} // DrvPrep


/*
** Name: prepare
**
** Description:
**	Prepare the statement and save the result-set meta-data.
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
**	10-Nov-06 (gordy)
**	    Created.
*/

public void
prepare()
    throws SQLException
{
    /*
    ** Prepare the statement
    */
    if ( trace.enabled( 2 ) )
	trace.write( tr_id + ": preparing statement '" + query + "'" );
    
    msg.lock();
    try
    {
	msg.begin( MSG_QUERY );
	msg.write( MSG_QRY_PREP );
	msg.write( MSG_QP_STMT_NAME );
	msg.write( stmtName );
	writeQueryText( query );
	msg.done( true );

	rsmd = readResults();
    }
    finally 
    { 
	msg.unlock(); 
    }

    return;
} // prepare


/*
** Name: getMetaData
**
** Description:
**	Retrieves the result-set meta-data describing the rows
**	returned by the prepared statement.  NULL is returned if
**	no ResultSet is associated with the prepared statement.
**	Meta-data is only available after statement is prepared.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	JdbcRSMD	The ResultSet meta-data, may be NULL.
**
** History:
**	 5-May-04 (gordy)
**	    Created.
*/

public JdbcRSMD
getMetaData()
{
    return( rsmd );
} // getMetadata


/*
** Name: getStmtName
**
** Description:
**	Returns the unique statement name by which the prepared
**	statement may be referenced in the server.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	Unique statement name.
**
** History:
**	 5-May-04 (gordy)
**	    Created.
*/

public String
getStmtName()
{
    return( stmtName );
} // getStmtName


/*
** Name: getTableName
**
** Description:
**	Returns the table name referenced in a SELECT statement.  The 
**	first table named in a FROM clause is returned exactly as it 
**	exists in the query text including schema and delimiters.  NULL
**	is returned if the statement is not a SELECT or FROM <table>
**	could not be parsed.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	Table name.
**
** History:
**	 5-May-04 (gordy)
**	    Created.
*/

public String
getTableName()
{
    return( tableName );
} // getTableName


/*
** Name: getConcurrency
**
** Description:
**	Returns the query concurrency based on presence of 'FOR UPDATE' 
**	or 'FOR READONLY' clause in SQL text.  If a concurrency is not
**	explicitly specified in the query text, DRV_CRSR_DBMS is
**	returned.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Concurrency: DRV_CRSR_{DBMS,READONLY,UPDATE}.
**
** History:
**	 5-May-04 (gordy)
**	    Created.
*/

public int
getConcurrency()
{
    return( concurrency );
} // getConcurrency


/*
** Name: attach
**
** Description:
**	Mark statement as actively associated with an extern 
**	reference.  Each attach() call must be follwed by one 
**	and only one accompanying call to detach().
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
**	10-Nov-06 (gordy)
**	    Created.
*/

public void
attach()
{
    refCount++;
    return;
} // attach


/*
** Name: detach
**
** Description:
**	Drop active association with statement.  Each detach() call
**	must have a single matching prior call to attach().
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
**	10-Nov-06 (gordy)
**	    Created.
*/

public void
detach()
{
    if ( refCount > 0 )  refCount--;
    return;
} // detach


/*
** Name: isActive
**
** Description:
**	Returns TRUE if an active association exists for this
**	statement (more calls to attach() than detach()).
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean		TRUE if active association exists.
**
** History:
**	10-Nov-06 (gordy)
**	    Created.
*/

public boolean
isActive()
{
    return( refCount > 0 );
} // isActive


/*
** Name: readDesc
**
** Description:
**	Read a query result DESC message.  Overrides the default 
**	method in DrvObj since descriptors are expected.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	JdbcRSMD    Query result data descriptor.
**
** History:
**	 5-May-04 (gordy)
**	    Created.
*/

protected JdbcRSMD
readDesc()
    throws SQLException
{
    return( JdbcRSMD.load( conn ) );
} // readDesc


} // class DrvPrep
