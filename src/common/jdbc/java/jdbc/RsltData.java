/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbc;

/*
** Name: RsltData.java
**
** Description:
**	Implements the EDBC JDBC ResultSet class RsltData
**	for handling fixed data sets.
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
*/

import	ca.edbc.util.EdbcEx;


/*
** Name: RsltData
**
** Description:
**	EDBC JDBC Java driver class which implements the
**	JDBC ResultSet interface for constant result data.
**
**  Implemented Methods:
**
**	load		    Load data subset.
**	shut		    Close result set.
**
**  Private Data:
**
**	loaded		    Has result set been loaded.
**
** History:
**	12-Nov-99 (rajus01)
**	    Created.
*/

class
RsltData
    extends EdbcRslt
{
   
    private boolean	loaded = false;		// Has result set been loaded?


/*
** Name: RsltData
**
** Description:
**	Class constructor.
**
** Input:
**	trace		Connection Tracing.
**	rsmd		ResultSetMetaData
**	data_set	Constant result set
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
*/

public
RsltData( EdbcTrace trace, EdbcRSMD rsmd, Object data_set[][] )
    throws EdbcEx
{
    super( trace, rsmd );
    tr_id = "Data[" + inst_id + "]";
    if ( (this.data_set = data_set) == null )  loaded = true;
} // RsltData


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
**	Load next data subset.  We have only the constant data set,
**	so we either use it, or set to an empty data set (end-of-data).
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
*/

protected void
load()
    throws EdbcEx
{
    if ( loaded )
	cur_row_cnt = 0;    // end of data set.
    else
    {
	/*
	** Data set saved by constructor,
	** make available now.
	*/
	cur_row_cnt = data_set.length;
	loaded = true;
    }
    
    cur_col_cnt = 0;	// never have partial row.
    return;
}


/*
** Name: shut
**
** Description:
**	Close result-set.  Simply mark data set as loaded.
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
**	12-Nov-99 (rajus01)
**	    Created.
**	28-Mar-01 (gordy)
**	    Functionality of close() added to shut().
*/

boolean // package access
shut()
    throws EdbcEx
{
    loaded = true;
    return( super.shut() );
} // shut

} // class RsltData

