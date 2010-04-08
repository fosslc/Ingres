/*
** Copyright (c) 1999, 2007 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: RsltData.java
**
** Description:
**	Defines class which implements the JDBC ResultSet interface
**	representing a fixed set of data.
**
**  Classes:
**
**	RsltData
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**	 9-May-00 (gordy)
**	    Reworked result set classes.  Extracted into own source file.
**	 4-Oct-00 (gordy)
**	    Standardized internal tracing.
**	 3-Nov-00 (gordy)
**	    Added support for JDBC 2.0 extensions.
**	28-Mar-01 (gordy)
**	    Tracing no longer globally accessible.
**	    Combined close() and shut().
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 4-Aug-03 (gordy)
**	    Moved row cacheing out of JdbcRslt.
**	26-Sep-03 (gordy)
**	    Column data now stored as SqlData objects.
**	 4-May-07 (gordy)
**	    Set class access for reflection.
**	20-Jul-07 (gordy)
**	    Row class now encapsulates a ResultSet row.
**      05-Jan-09 (rajus01) SIR 121238
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to
**            support JDBC 4.0 SQLException hierarchy.
*/

import	java.sql.SQLException;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.util.SqlData;


/*
** Name: RsltData
**
** Description:
**	JDBC driver class implementing the JDBC ResultSet interface
**	for a fixed pre-defined set of data.
**
**  Methods Overridden:
**
**	load		    Load next row column set.
**
**  Private Data:
**
**	dataSet		    Row/column data
**	empty		    Empty row/column set.
**
** History:
**	12-Nov-99 (rajus01)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 4-Aug-03 (gordy)
**	    Row cache implemented with addition of data_set, so no longer
**	    need loaded indicator which also allowed shut() to be dropped.
**	    Super-class now only requires single load() method.
**	26-Sep-03 (gordy)
**	    Column data now stored as SqlData objects.  Renamed data_set
**	    to dataSet.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, permit access.
*/

public class
RsltData
    extends JdbcRslt
{
   
    /*
    ** Result set containing rows with column values.
    */
    private SqlData			dataSet[][] = null;

    /*
    ** Empty result set if initialized with null set.
    */
    private static final SqlData	empty[][] = new SqlData[0][];
    
    
/*
** Name: RsltData
**
** Description:
**	Class constructor.
**
** Input:
**	conn		Associated connection.
**	rsmd		ResultSet meta-data
**	dataSet		Constant result set, may be null
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	12-Nov-99 (rajus01)
**	    Created.
**	 4-Oct-00 (gordy)
**	    Create unique ID for standardized internal tracing.
**	28-Mar-01 (gordy)
**	    Tracing added as a parameter.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	26-Sep-03 (gordy)
**	    Column values now stored as SQL data objects.
**	26-Sep-03 (gordy)
**	    Column data now stored as SqlData objects.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, restrict constructor access.
**	20-Jul-07 (gordy)
**	    Set the size of the result-set.
*/

// package access
RsltData( DrvConn conn, JdbcRSMD rsmd, SqlData dataSet[][] )
    throws SQLException
{
    super( conn, rsmd );
    this.dataSet = (dataSet == null) ? empty : dataSet;
    rowCount = this.dataSet.length;
    tr_id = "Data[" + inst_id + "]";
} // RsltData


/*
** Name: load
**
** Description:
**	Load column set for next row.
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
**	12-Nov-99 (rajus01)
**	    Created.
**	 4-Aug-03 (gordy)
**	    Row caching support moved to this class.
**	20-Jul-07 (gordy)
**	    Row class now encapsulates a ResultSet row.
*/

protected void
load()
    throws SQLException
{
    /*
    ** Since base class guarantees the current row state to
    ** be consistent and valid, our task is very easy.  We
    ** are moving forward one row either to a valid row or
    ** past the end of the result set.  If we are moving to
    ** a valid row, we may be moving from before the first
    ** or from another row and may land on the last row.
    */
    switch( posStatus )
    {
    case BEFORE :
	if ( dataSet.length == 0 )
	    posStatus = AFTER;
	else
	{
	    currentRow = new Row();
	    currentRow.id = 1;
	    currentRow.columns = dataSet[ 0 ];
	    currentRow.count = currentRow.columns.length;
	    currentRow.status = (dataSet.length == 1) 
				? Row.FIRST | Row.LAST : Row.FIRST;
	    posStatus = ROW;
	}
	break;

    case ROW :
	if ( currentRow.id >= dataSet.length )
	    posStatus = AFTER;
	else
	{
	    currentRow.columns = dataSet[ currentRow.id ];
	    currentRow.count = currentRow.columns.length;
	    currentRow.id++;
	    currentRow.status = (currentRow.id == dataSet.length) 
				? Row.LAST : 0;
	    posStatus = ROW;
	}
	break;
    }

    return;
}


} // class RsltData
