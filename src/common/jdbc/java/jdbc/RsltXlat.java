/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbc;

/*
** Name: RsltXlat.java
**
** Description:
**	Implements the EDBC JDBC ResultSet class RsltXlat
**	for handling data sets translated from another set.
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
*/

import	java.sql.Types;
import	java.sql.SQLException;
import	ca.edbc.util.EdbcEx;


/*
** Name: RsltXlat
**
** Description:
**	EDBC JDBC Java driver class which implements the
**	JDBC ResultSet interface for translated data sets.
**
**  Implemented Methods:
**
**	load		    Load data subset.
**	shut		    Close result set.
**
**  Protected Data:
**
**	dbmsRS		    Result set to be translated.
**
**  Protectdd Methods
**
**	load_data	    Load/translate columns of data set.
**	convToJavaType	    Converts DBMS type java.sql type
** 	colSize		    Returns DBMS MetaData COLUMN_SIZE for
**			    a given java.sql type.
** History:
**	 9-Dec-99 (gordy)
**	    Created.
**	22-Dec-99 (rajus01)
**	    Added convToJavaType, colSize methods..
*/

class
RsltXlat
    extends EdbcRslt
{

    protected	EdbcRslt	dbmsRS = null;


/*
** Name: RsltXlat
**
** Description:
**	Class constructor.
**
** Input:
**	trace		Connection Tracing.
**	rsmd		Result-set Meta-Data.
**	dbmsRS		Result-set to translate.
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
*/

public
RsltXlat( EdbcTrace trace, EdbcRSMD rsmd, EdbcRslt dbmsRS )
    throws EdbcEx
{
    super( trace, rsmd );
    tr_id = "Xlat[" + inst_id + "]";
    data_set = new Object[ 1 ][];
    data_set[ 0 ] = new Object[ rsmd.count ];
    this.dbmsRS = dbmsRS;
} // RsltXlat


/*
** Name: load
**
** Description:
**	Load columns (during partial row processing) at least 
**	upto the requested column.  Partial row fetches are
**	not supported, so this method should never be called.
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
*/

protected void
load( int index )
    throws EdbcEx
{
    cur_col_cnt = 0;	// Should never be called.
    return;
}


/*
** Name: load
**
** Description:
**	Load next data row.  First we get the next row of the
**	associated data set, then each column of this data set
**	is loaded/translated from the associated data row.
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
*/

protected void
load()
    throws EdbcEx
{
    try
    {
	if ( ! dbmsRS.next() )  
	    cur_row_cnt = 0;
	else
	{
	    load_data();
	    cur_row_cnt = 1;
	}
    }
    catch( SQLException ex )
    { 
	throw (EdbcEx)ex; 
    }

    cur_col_cnt = 0;	// Partial row fetches not supported.
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
    throws EdbcEx
{
    boolean closing;

    if ( (closing = super.shut()) )
	try { dbmsRS.close(); }
	catch( SQLException ex )
	{ throw (EdbcEx)ex; }

    return( closing );
} // shut


/*
** Name: load_data
**
** Description:
**	Default row loading method.  This method should be overriden
**	to provide the required conversion for a translated result set.
**
**	Default action is to convert the DBMS datatype to the Meta-Data
**	datatype with a direct linear mapping of columns.
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
** Name: convert
**
** Description:
**	Convert a DBMS datatype (from the DBMS result set)
**	to a Meta-Data datatype (in our local data set).
**
** Input:
**	col	    Meta-Data column (0 based).
**	dbms_col    DBMS column (1 based).
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
*/

protected void
convert( int col, int dbms_col )
    throws SQLException
{
    /*
    ** There are only a limited number of SQL types
    ** actually used in Meta-Data result sets.
    */
    switch( rsmd.desc[ col ].sql_type )
    {
    case Types.SMALLINT :
	short s = dbmsRS.getShort( dbms_col );
	if ( ! dbmsRS.wasNull() )  
	    data_set[ 0 ][ col ] = new Short( s );
	else
	    data_set[ 0 ][ col ] = null;
	break;

    case Types.INTEGER :
	int i = dbmsRS.getInt( dbms_col );
	if ( ! dbmsRS.wasNull() )  
	    data_set[ 0 ][ col ] = new Integer( i );
	else
	    data_set[ 0 ][ col ] = null;
	break;

    case Types.VARCHAR : 
        String str = dbmsRS.getString(dbms_col);
	if ( str != null )
	    data_set[ 0 ][ col ] = str.trim();
	else
	    data_set[ 0 ][ col ] = null;
	break;

    default : throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
    }

    return;
}

} // class RsltXlat
