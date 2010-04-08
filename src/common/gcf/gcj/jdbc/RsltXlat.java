/*
** Copyright (c) 1999, 2009 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: RsltXlat.java
**
** Description:
**	Defines class which implements JDBC ResultSet interface
**	representing a translation of some other result-set.
**
**  Classes:
**
**	RsltXlat
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**	 4-Oct-00 (gordy)
**	    Standardized internal tracing.
**	15-Nov-00 (gordy)
**	    Added support for JDBC 2.0 extensions.
**	28-Mar-01 (gordy)
**	    Tracing no longer global accessible.  
**	    Combined close() and shut().
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 4-Aug-03 (gordy)
**	    Removed row cacheing from JdbcRslt.
**	26-Sep-03 (gordy)
**	    Column values now stored as SqlData objects.
**	 4-May-07 (gordy)
**	    Set class access for reflection.
**	20-Jul-07 (gordy)
**	    Row class now encapsulates a ResultSet row.
**      05-Jan-09 (rajus01) SIR 121238
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to
**            support JDBC 4.0 SQLException hierarchy.
*/

import	java.sql.Types;
import	java.sql.SQLException;
import	com.ingres.gcf.util.SqlData;
import	com.ingres.gcf.util.SqlBool;
import	com.ingres.gcf.util.SqlTinyInt;
import	com.ingres.gcf.util.SqlSmallInt;
import	com.ingres.gcf.util.SqlInt;
import	com.ingres.gcf.util.SqlBigInt;
import	com.ingres.gcf.util.SqlReal;
import	com.ingres.gcf.util.SqlDouble;
import	com.ingres.gcf.util.SqlDecimal;
import	com.ingres.gcf.util.SqlString;
import	com.ingres.gcf.util.SqlExFactory;


/*
** Name: RsltXlat
**
** Description:
**	JDBC driver class which implements the JDBC ResultSet interface 
**	for a result-set which is a translation of some other result-set.
**
**	Currently only BOOLEAN, TINYINT, SMALLINT, INTEGER, BIGINT, REAL, 
**	DOUBLE, DECIMAL, and VARCHAR are supported as exposed column types.
**
**	By default, the exposed result-set must have a one-to-one column
**	association with the underlying result-set and the column types
**	must be restricted to SMALLINT, INTEGER, and VARCHAR.  Sub-
**	classes must override the load_data() method to provide other 
**	column or type mappings.
**
**  Methods Overriden:
**
**	load			Load data subset.
**	shut			Close result-set.
**
**  Protected Data:
**
**	dbmsRS			ResultSet to be translated.
**
**  Protected Methods:
**
**	load_data		Load/translate columns of data set.
**	setNull			Set column NULL.
**	setBoolean		Assign boolean to column.
**	setShort		Assign short to column.
**	setInt			Assign integer to column.
**	setString		Assign string to column.
**	convert			Load a column converting to local type.
**
**  Private Methods:
**
**	markColumnAvailable	Mark column loaded and available.
**	allocateRowBuffer	Allocate row/column data buffers.
**
** History:
**	 9-Dec-99 (gordy)
**	    Created.
**	22-Dec-99 (rajus01)
**	    Added convToJavaType, colSize methods.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 4-Aug-03 (gordy)
**	    No longer need to implement partial row load method.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, permit access.
**	20-Jul-07 (gordy)
**	    Add methods for assigning column values.
*/

public class
RsltXlat
    extends JdbcRslt
{

    protected	JdbcRslt	dbmsRS = null;


/*
** Name: RsltXlat
**
** Description:
**	Class constructor.
**
** Input:
**	conn		Associated connection.
**	rsmd		ResultSet meta-data.
**	dbmsRS		ResultSet to translate.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 9-Dec-99 (gordy)
**	    Created.
**	 4-Oct-00 (gordy)
**	    Create unique ID for standardized internal tracing.
**	28-Mar-01 (gordy)
**	    Tracing added as a parameter.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 4-Aug-03 (gordy)
**	    Row cacheing removed from base class.
**	26-Sep-03 (gordy)
**	    Column values now stored as SqlData objects.  Extracted
**	    column array allocation to allocateRowBuffer().
**	 4-May-07 (gordy)
**	    Class is exposed outside package, restrict constructor access.
*/

// package access
RsltXlat( DrvConn conn, JdbcRSMD rsmd, JdbcRslt dbmsRS )
    throws SQLException
{
    super( conn, rsmd );
    this.dbmsRS = dbmsRS;
    tr_id = "Xlat[" + inst_id + "]";
    return;
} // RsltXlat


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
**	void
**
** History:
**	 8-Dec-99 (gordy)
**	    Created.
**	 4-Aug-03 (gordy)
**	    Row caching removed from base class.  Row state
**	    derived from underlying result-set
**	20-Jul-07 (gordy)
**	    Row class now encapsulates a ResultSet row.
**	02-Feb-09 (rajus01) SIR 121238
**	    Code cleanup: Removed try...catch() block as SqlEx is gone.
**	
*/

protected void
load()
    throws SQLException
{
    /*
    ** Attempt to read row from underlying result-set
    ** and set our row state accordingly.
    */
    switch( posStatus )
    {
	case BEFORE :
	    if ( ! dbmsRS.next() )
	    {
		posStatus = AFTER;
		rowCount = 0;
	    }
	    else
	    {
		currentRow = new Row();
		currentRow.id = 1;
		currentRow.columns = allocateRowBuffer( rsmd );
		currentRow.count = 0;

		if ( ! dbmsRS.isLast() )  
		    currentRow.status = Row.FIRST;
		else
		{
		    currentRow.status = Row.FIRST | Row.LAST;
		    rowCount = currentRow.id;
		}

		load_data();
		posStatus = ROW;
	    }
	    break;

	case ROW :
	    if ( ! dbmsRS.next() )
	    {
		posStatus = AFTER;
		rowCount = currentRow.id;
	    }
	    else
	    {
		currentRow.id++;
		currentRow.count = 0;

		if ( ! dbmsRS.isLast() )  
		    currentRow.status = 0;
		else
		{
		    currentRow.status = Row.LAST;
		    rowCount = currentRow.id;
		}

		load_data();
		posStatus = ROW;
	    }
	    break;
    }

    return;
}


/*
** Name: shut
**
** Description:
**	Close the result-set.
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
**	 8-Dec-99 (gordy)
**	    Created.
**	28-Mar-01 (gordy)
**	    Functionality of close() added to shut().
*/

boolean	// package access
shut()
    throws SQLException
{
    boolean closing;

    if ( (closing = super.shut()) )
	try { dbmsRS.close(); }
	catch( SQLException ex ) { throw ex; }

    return( closing );
} // shut


/*
** Name: load_data
**
** Description:
**	Default row loading method.  This method should be overriden
**	to provide the required conversion for a translated result set.
**
**	Default action is to convert the underlying column value to  
**	the exposed data type, as supported by the convert() method, 
**	with a linear mapping of columns.
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
**	 9-Dec-99 (gordy)
**	    Created.
*/

protected void
load_data()
    throws SQLException
{
    for( int i = 0; i < rsmd.count; i++ )  convert( i, i + 1 );
    return;
}


/*
** Name: setNull
**
** Description:
**	Set a column in the current row to NULL.
**
** Input:
**	col	Column number (0 based).
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	20-Jul-07 (gordy)
**	    Created.
*/

protected void
setNull( int col )
    throws SQLException
{
    currentRow.columns[ col ].setNull();
    markColumnAvailable( col );
    return;
} // setNull


/*
** Name: setBoolean
**
** Description:
**	Assign a boolean value to a column in the current row.
**
** Input:
**	col	Column number (0 based).
**	value	Value to be assigned.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	20-Jul-07 (gordy)
**	    Created.
*/

protected void
setBoolean( int col, boolean value )
    throws SQLException
{
    ((SqlBool)currentRow.columns[ col ]).set( value );
    markColumnAvailable( col );
    return;
} // setShort


/*
** Name: setShort
**
** Description:
**	Assign a short value to a column in the current row.
**
** Input:
**	col	Column number (0 based).
**	value	Value to be assigned.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	20-Jul-07 (gordy)
**	    Created.
*/

protected void
setShort( int col, short value )
    throws SQLException
{
    ((SqlSmallInt)currentRow.columns[ col ]).set( value );
    markColumnAvailable( col );
    return;
} // setShort


/*
** Name: setInt
**
** Description:
**	Assign an integer value to a column in the current row.
**
** Input:
**	col	Column number (0 based).
**	value	Value to be assigned.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	20-Jul-07 (gordy)
**	    Created.
*/

protected void
setInt( int col, int value )
    throws SQLException
{
    ((SqlInt)currentRow.columns[ col ]).set( value );
    markColumnAvailable( col );
    return;
} // setInt


/*
** Name: setString
**
** Description:
**	Assign a string value to a column in the current row.
**
** Input:
**	col	Column number (0 based).
**	value	Value to be assigned.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	20-Jul-07 (gordy)
**	    Created.
*/

protected void
setString( int col, String value )
    throws SQLException
{
    ((SqlString)currentRow.columns[ col ]).set( value );
    markColumnAvailable( col );
    return;
} // setString


/*
** Name: convert
**
** Description:
**	Load a column value from the underlying result-set with
**	conversion to the exposed column data type.  Currently,
**	the only supported data types for the exposed column are
**	SMALLINT, INTEGER and VARCHAR (strings are trimmed!).
**
** Input:
**	col	    Exposed result-set column (0 based).
**	dbms_col    Underlying column (1 based).
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 9-Dec-99 (gordy)
**	    Created.
**	22-Dec-99 (rajus01)
**	    Removed reference 'str'.
**	16-Feb-00 (rajus01)
**	    Applied trim method to remove white spaces for
**	    VARCHAR column type.
**	25-Feb-00 (rajus01)
**	    Test for null before applying trim() method for 
**	    VARCHAR column type.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 4-Aug-03 (gordy)
**	    Only one row now maintained.
**	26-Sep-03 (gordy)
**	    Column values now stored as SqlData objects.
*/

protected void
convert( int col, int dbms_col )
    throws SQLException
{
    switch( rsmd.desc[ col ].sql_type )
    {
    case Types.SMALLINT :
	short s = dbmsRS.getShort( dbms_col );
	if ( ! dbmsRS.wasNull() )  
	    ((SqlSmallInt)currentRow.columns[ col ]).set( s );
	else
	    currentRow.columns[ col ].setNull();
	break;

    case Types.INTEGER :
	int i = dbmsRS.getInt( dbms_col );
	if ( ! dbmsRS.wasNull() )  
	    ((SqlInt)currentRow.columns[ col ]).set( i );
	else
	    currentRow.columns[ col ].setNull();
	break;

    case Types.VARCHAR : 
        String str = dbmsRS.getString( dbms_col );
	if ( str != null )
	    ((SqlString)currentRow.columns[ col ]).set( str.trim() );
	else
	    currentRow.columns[ col ].setNull();
	break;

    default : throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
    }

    markColumnAvailable( col );
    return;
}


/*
** Name: markColumnAvailable
**
** Description:
**	Marks the column as being loaded and available.
**
**	Note: individual columns aren't actually identified
**	as being loaded/available, so the highest available
**	column is marked.  It is the responsibility of the
**	sub-class to ensure that all preceding columns are
**	loaded.
**
** Input:
**	col	Column index (0 based).
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	20-Jul-07 (gordy)
**	    Created.
*/

private void
markColumnAvailable( int col )
{
    currentRow.count = Math.max( col + 1, currentRow.count );
    return;
} // markColumnAvailable


/*
** Name: allocateRowBuffer
**
** Description:
**	Allocates the row buffer and populates the column
**	entries with SQL data objects matching the column
**	type.
**
** Input:
**	rsmd		Result-set Meta-data.
**
** Output:
**	None.
**
** Returns:
**	SqlData[]	Row buffer.
**
** History:
**	26-Sep-03 (gordy)
**	    Created.
**	 9-Sep-08 (gordy)
**	    Set max decimal precision.
*/

private SqlData[]
allocateRowBuffer( JdbcRSMD rsmd )
    throws SQLException
{
    SqlData row[] = new SqlData[ rsmd.count ];

    for( int col = 0; col < rsmd.count; col++ )
    {
        switch( rsmd.desc[ col ].sql_type )
        {
	case Types.BOOLEAN :	row[ col ] = new SqlBool();	break;
        case Types.TINYINT :	row[ col ] = new SqlTinyInt();	break;
        case Types.SMALLINT :	row[ col ] = new SqlSmallInt();	break;
        case Types.INTEGER :	row[ col ] = new SqlInt();	break;
	case Types.BIGINT :	row[ col ] = new SqlBigInt();	break;
        case Types.REAL :	row[ col ] = new SqlReal();	break;
        case Types.DOUBLE :	row[ col ] = new SqlDouble();	break;
        case Types.VARCHAR :    row[ col ] = new SqlString();	break;
        
        case Types.DECIMAL :	
	    row[ col ] = new SqlDecimal( conn.max_dec_prec );
	    break;

	default :	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
        }
    }

    return( row );
}


} // class RsltXlat
