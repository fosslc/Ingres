/*
** Copyright (c) 2003, 2009 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: RsltUpd.java
**
** Description:
**	Defines class which implements the JDBC ResultSet interface
**	and supports updatable cursors.
**
**  Classes
**
**	RsltUpd
**
** History:
**	 4-Aug-03 (gordy)
**	    Created.
**	 6-Oct-03 (gordy)
**	    Added pre-loading ability to result-sets.
**	 1-Nov-03 (gordy)
**	    Implemented actual JDBC support for updatable result-sets.
**	15-Mar-04 (gordy)
**	    Support BIGINT columns.
**	23-Apr-04 (wansh01)
**	    in updateXXX methods, if input value is null and the field is not
**  	    nulllable throw ERR_GC401A_CONVERSION_ERR.   
**	    Moved columnMap() from initParam() to individual calling rtns.
**	28-Apr-04 (wansh01)
**	    Added type as an input for setObejct().
**	19-Jun-06 (gordy)
**	    ANSI Date/Time data type support.
**	25-Aug-06 (gordy)
**	    JdbcRSMD enhanced for parameter support.
**	 9-Nov-06 (gordy)
**	    Handle query text over 64K.
**	15-Nov-06 (gordy)
**	    Support LOB Locators via BLOB/CLOB.
**	26-Feb-07 (gordy)
**	    Support Locator coercions by initializing parameters associated
**	    with Locator columns to LOB types by default.  Instances where
**	    Locators are applicable already perform direct initialization.
**	    Neither Locator nor LOB values are copied back to the source
**	    row, so this change only affects parameter handling.
**	 6-Apr-07 (gordy)
**	    Support scrollable cursors.
**	 4-May-07 (gordy)
**	    Set class access for reflection.
**	20-Jul-07 (gordy)
**	    Row class now encapsulates a ResultSet row.
**      05-Jan-09 (rajus01) SIR 121238
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to
**            support JDBC 4.0 SQLException hierarchy.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	16-Mar-09 (gordy)
**	    Cleanup handling of internal LOBs.
**      7-Apr-09 (rajus01) SIR 121238
**          Added new JDBC 4.0 update methods.
**	22-Jun-09 (rajus01) SIR 121238
**	    Fixed the data type of length parameter in update*Stream() methods.
**	24-Jun-09 (rajus01) SIR 121238
**	    Fixed the wrapped methods to take length parameter from the 
**	    respective wrappers for update*Stream() methods.
**	26-Oct-09 (gordy)
**	    Added support for BOOLEAN data type.
*/

import	java.math.BigDecimal;
import	java.io.InputStream;
import	java.io.Reader;
import	java.sql.ResultSet;
import	java.sql.Types;
import	java.sql.Date;
import	java.sql.Time;
import	java.sql.Timestamp;
import	java.sql.Blob;
import	java.sql.Clob;
import	java.sql.NClob;
import	java.sql.SQLException;
import	com.ingres.gcf.util.DbmsConst;
import	com.ingres.gcf.util.GcfErr;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.util.SqlData;
import	com.ingres.gcf.util.SqlTinyInt;
import	com.ingres.gcf.util.SqlSmallInt;
import	com.ingres.gcf.util.SqlInt;
import	com.ingres.gcf.util.SqlBigInt;
import	com.ingres.gcf.util.SqlReal;
import	com.ingres.gcf.util.SqlDouble;
import	com.ingres.gcf.util.SqlDecimal;
import	com.ingres.gcf.util.SqlBool;
import  com.ingres.gcf.util.SqlDate;
import  com.ingres.gcf.util.SqlTime;
import  com.ingres.gcf.util.SqlTimestamp;
import	com.ingres.gcf.util.IngresDate;
import	com.ingres.gcf.util.SqlByte;
import	com.ingres.gcf.util.SqlVarByte;
import	com.ingres.gcf.util.SqlChar;
import	com.ingres.gcf.util.SqlNChar;
import	com.ingres.gcf.util.SqlVarChar;
import	com.ingres.gcf.util.SqlNVarChar;
import	com.ingres.gcf.dam.MsgConst;


/*
** Name: RsltUpd
**
** Description:
**	JDBC driver class which implements the JDBC ResultSet interface
**	and adds support for updatable cursors.
**
**	Maintains a parameter set for columns updated or to be inserted.
**	The JDBC insert row is virtualized and position on the row is
**	simply indicated by a flag.  All operations sensitive to the
**	insert row simply test the condition of the insert flag.
**
**	Row updates and deletes are executed as cursor positioned updates
**	and deletes.  Row inserts are executed as simple insert statements.
**
**  Interface Methods
**
**	moveToInsertRow		Position on (virtual) insert row.
**	moveToCurrentRow	Position on actual result-set row.
**	cancelRowUpdates	Cancel updates to current row.
**	updateNull		Set parameter to a NULL value.
**	updateBoolean		Set parameter to a boolean value.
**	updateByte		Set parameter to a byte value.
**	updateShort		Set parameter to a short value.
**	updateInt		Set parameter to a int value.
**	updateLong		Set parameter to a long value.
**	updateFloat		Set parameter to a float value.
**	updateDouble		Set parameter to a double value.
**	updateBigDecimal	Set parameter to a BigDecimal value.
**	updateString		Set parameter to a string value.
**	updateBytes		Set parameter to a byte array value.
**	updateDate		Set parameter to a Date value.
**	updateTime		Set parameter to a Time value.
**	updateTimestamp		Set parameter to a Timestamp value.
**	updateBinaryStream	Set parameter to a binary byte stream.
**	updateAsciiStream	Set parameter to a ASCII character stream.
**	updateCharacterStream	Set parameter to a Character stream.
**	updateObject		Set parameter to a generic Java object.
**	deleteRow		Delete current row.
**	updateRow		Update current row.
**	insertRow		Insert new row.
**
**  Overridden Methods:
**
**	load			Load next row-set.
**	getRow			Returns current row number.
**	isBeforeFirst		Is cursor before first row?
**	isAfterLast		Is cursor after last row?
**	isFirst			Is cursor on first row?
**	isLast			Is cursor on last row?
**	rowUpdated		Has current row been updated?
**	rowDeleted		Has current row been deleted?
**	rowInserted		Has current row been inserted?
**	refreshRow		Reload current row column values.
**	columnMap		Validates column index, convert to internal.
**	getSqlData		Retrieve column data value.
**
**  Protected Data
**
**	table			Updatable table name.
**	params			Parameters for update/insert.
**	insert			Positioned on (virtual) insert row?
**
**  Private Methods
**
**	initParams		Initialize parameters based on column types.
**	copyParams		Copy parameters to row column data.
**	exec			Execute a row update, delete, or insert.
**
** History:
**	 4-Aug-03 (gordy)
**	    Created.
**	 1-Nov-03 (gordy)
**	    Implemented support for updatables result-sets.
**	15-Nov-06 (gordy)
**	    Implemented updateBlob() and updateClob() to support
**	    DBMS LOB locators.
**	 6-Apr-07 (gordy)
**	    Exposed data to sub-classes.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, permit access.
*/

public class
RsltUpd
    extends RsltCurs
    implements MsgConst, DbmsConst, GcfErr
{

    protected String	table = null;		// Updatable table name.
    protected ParamSet	params = null;		// Update/Insert parameters.
    protected boolean	insert = false;		// Position on insert row?
    

/*
** Name: RsltUpd
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
**	 4-Aug-03 (gordy)
**	    Created.
**	 6-Oct-03 (gordy)
**	    Disable pre-loading in super-class.
**	 1-Nov-03 (gordy)
**	    Added table name parameter for cursor update/delete.
**	    Flag as updatable and allocate parameter set.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, restrict constructor access.
**	20-Jul-07 (gordy)
**	    Pre-fetching now restricted via fetch size limit.
*/

// package access
RsltUpd
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
    /*
    ** Updatable cursors only permit a single row per fetch,
    ** so pre-fetching and pre-loading are disabled.
    */
    super( conn, stmt, rsmd, stmt_id, cursor, -1, false );
    rs_concur = ResultSet.CONCUR_UPDATABLE;
    
    this.table = table;
    params = new ParamSet( conn, rsmd.count );
    tr_id = "Upd[" + inst_id + "]";
    return;
} // RsltUpd


/*
** Name: load
**
** Description:
**	Position the result set to the next row and load the column
**	values.  
**
**	Clears row updates and calls super-class to load next row.
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
**	 1-Nov-03 (gordy)
**	    Created.
*/

protected void
load()
    throws SQLException
{
    if ( insert )
    {
	if ( trace.enabled(1) )  trace.write( tr_id + ".load(): invalid row" );
	throw SqlExFactory.get( ERR_GC4021_INVALID_ROW ); 
    }
    
    params.clear( false );
    super.load();
    return;
} // load


/*
** Name: moveToInsertRow
**
** Description:
**	Conceptually moves cursor to insert row.
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
**	 1-Nov-03 (gordy)
**	    Created.
**	 6-Apr-07 (gordy)
**	    Flush current row to clear processing state.
*/

public synchronized void 
moveToInsertRow() 
    throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".moveToInsertRow()" );
    
    if ( ! insert )
    {
	/*
	** Current row must be flushed.
	*/
	flush();
	params.clear( false );
	insert = true;
    }
    return;
} // moveToInsertRow


/*
** Name: moveToCurrentRow
**
** Description:
**	Move cursor to current row.  Only relevant if cursor
**	is currently on (virtual) insert row.
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
**	 1-Nov-03 (gordy)
**	    Created.
*/

public synchronized void 
moveToCurrentRow() 
    throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".moveToCurrentRow()" );
    
    if ( insert )
    {
	params.clear( false );
	insert = false;
    }
    return;
} // moveToCurrentRow


/*
** Name: cancelRowUpdates
**
** Description:
**	Cancels updates made to current row (clears parameter set).
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
**	 1-Nov-03 (gordy)
**	    Created.
*/

public void
cancelRowUpdates() 
    throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".cancelRowUpdates()" ); 
    params.clear( false );
    return;
} // cancelRowUpdates


/*
** The following methods are not allowed on insert row.
** Otherwise, super-class implementation is used.
*/

public int
getRow() throws SQLException
{
    if ( insert )
    { 
	if ( trace.enabled() )  trace.log( title + ".getRow(): insert row!" );
	throw SqlExFactory.get( ERR_GC4021_INVALID_ROW ); 
    }
    
    return( super.getRow() );
} // getRow


public boolean
isBeforeFirst()
    throws SQLException
{
    if ( insert )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".isBeforeFirst(): insert row!" );
	throw SqlExFactory.get( ERR_GC4021_INVALID_ROW ); 
    }
    
    return( super.isBeforeFirst() );
} // isBeforeFirst


public boolean
isAfterLast() throws SQLException
{
    if ( insert )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".isAfterLast(): insert row!" );
	throw SqlExFactory.get( ERR_GC4021_INVALID_ROW ); 
    }
    
    return( super.isAfterLast() );
} // isAfterLast


public boolean
isFirst() throws SQLException
{
    if ( insert )
    {
	if ( trace.enabled() )  trace.log( title + ".isFirst(): insert row!" );
	throw SqlExFactory.get( ERR_GC4021_INVALID_ROW ); 
    }
    
    return( super.isFirst() );
} // isFirst


public boolean
isLast() throws SQLException
{
    if ( insert )
    {
	if ( trace.enabled() )  trace.log( title + ".isLast(): insert row!" );
	throw SqlExFactory.get( ERR_GC4021_INVALID_ROW ); 
    }
    
    return( super.isLast() );
} // isLast


public boolean 
rowUpdated() throws SQLException
{ 
    if ( insert )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".rowUpdated(): insert row!" );
	throw SqlExFactory.get( ERR_GC4021_INVALID_ROW ); 
    }
    
    return( super.rowUpdated() ); 
} // rowUpdated


public boolean 
rowDeleted() throws SQLException
{ 
    if ( insert )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".rowDeleted(): insert row!" );
	throw SqlExFactory.get( ERR_GC4021_INVALID_ROW ); 
    }
    
    return( super.rowDeleted() ); 
} // rowDeleted


public boolean 
rowInserted() throws SQLException
{ 
    if ( insert )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".rowInserted(): insert row!" );
	throw SqlExFactory.get( ERR_GC4021_INVALID_ROW ); 
    }
    
    return( super.rowInserted() ); 
} // rowInserted


/*
** Name: refreshRow
**
** Description:
**	Re-load column values for current row.
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
**	 1-Nov-03 (gordy)
**	    Implemented.
*/

public void 
refreshRow() 
    throws SQLException
{ 
    if ( insert )
    {
	if ( trace.enabled() )  trace.log(title + ".refreshRow(): invalid row");
	throw SqlExFactory.get( ERR_GC4021_INVALID_ROW ); 
    }
    
    params.clear( false ); 
    super.refreshRow();
    return;
} // refreshRow


/*
** Name: updateNull
**
** Description:
**	Set column to a NULL value.
**
** Input:
**	index	Column index.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-03 (gordy)
**	    Implemented.
**	23-Apr-04 (wansh01)
**	    if field is not nullable throw ERR_GC401A_CONVERSION_ERR.   
**	25-Aug-06 (gordy)
**	    Descriptor changed to hold mode flags.
*/

public void
updateNull( int index ) 
    throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".updateNull(" + index + ")" ); 
	
    index = columnMap( index );
    if ( (rsmd.desc[ index ].flags & MSG_DSC_NULL) == 0 )
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
    initParam( index );
    params.setNull( index );
    return;
} // updateNull


/*
** Name: updateBoolean
**
** Description:
**	Set column to a boolean value.
**
** Input:
**	index	Column index.
**	value	Boolean value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-03 (gordy)
**	    Implemented.
**	23-Apr-04 (wansh01)
**	    Added columnMap() before calling initParms. 
*/

public void
updateBoolean( int index, boolean value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log( title + ".updateBoolean( " + index + "," + value + ")" );
    
    index = columnMap( index );
    initParam( index );
    params.setBoolean( index, value );
    return;
} // updateBoolean


/*
** Name: updateByte
**
** Description:
**	Set column to a byte value.
**
** Input:
**	index	Column index.
**	value	Byte value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-03 (gordy)
**	    Implemented.
**	23-Apr-04 (wansh01)
**	    Added columnMap() before calling initParms. 
*/

public void
updateByte( int index, byte value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log( title + ".updateByte(" + index + "," + value + ")" );
    
    index = columnMap( index );
    initParam( index );
    params.setByte( index, value );
    return;
} // updateByte


/*
** Name: updateShort
**
** Description:
**	Set column to a short value.
**
** Input:
**	index	Column index.
**	value	Short value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-03 (gordy)
**	    Implemented.
**	23-Apr-04 (wansh01)
**	    Added columnMap() before calling initParms. 
*/

public void
updateShort( int index, short value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log( title + ".updateShort(" + index + "," + value + ")" );
    
    index = columnMap( index );
    initParam( index );
    params.setShort( index, value );
    return;
} // updateShort


/*
** Name: updateInt
**
** Description:
**	Set column to a int value.
**
** Input:
**	index	Column index.
**	value	Integer value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-03 (gordy)
**	    Implemented.
**	23-Apr-04 (wansh01)
**	    Added columnMap() before calling initParms. 
*/

public void
updateInt( int index, int value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log( title + ".updateInt(" + index + "," + value + ")");
    
    index = columnMap( index );
    initParam( index );
    params.setInt( index, value );
    return;
} // updateInt


/*
** Name: updateLong
**
** Description:
**	Set column to a long value.
**
** Input:
**	index	Column index.
**	value	Long value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-03 (gordy)
**	    Implemented.
**	23-Apr-04 (wansh01)
**	    Added columnMap() before calling initParms. 
*/

public void
updateLong( int index, long value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log( title + ".updateLong(" + index + "," + value + ")" );
    
    index = columnMap( index );
    initParam( index );
    params.setLong( index, value );
    return;
} // updateLong


/*
** Name: updateFloat
**
** Description:
**	Set column to a float value.
**
** Input:
**	index	Column index.
**	value	Float value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-03 (gordy)
**	    Implemented.
**	23-Apr-04 (wansh01)
**	    Added columnMap() before calling initParms. 
*/

public void
updateFloat( int index, float value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log( title + ".updateFloat(" + index + "," + value + ")" );
    
    index = columnMap( index );
    initParam( index );
    params.setFloat( index, value );
    return;
} // updateFloat


/*
** Name: updateDouble
**
** Description:
**	Set column to a double value.
**
** Input:
**	index	Column index.
**	value	Double value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-03 (gordy)
**	    Implemented.
**	23-Apr-04 (wansh01)
**	    Added columnMap() before calling initParms. 
*/

public void
updateDouble( int index, double value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log( title + ".updateDouble(" + index + "," + value + ")" );
    
    index = columnMap( index );
    initParam( index );
    params.setDouble( index, value );
    return;
} // updateDouble


/*
** Name: updateBigDecimal
**
** Description:
**	Set column to a BigDecimal value.
**
** Input:
**	index	Column index.
**	value	BigDecimal value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-03 (gordy)
**	    Implemented.
**	23-Apr-04 (wansh01)
**	    if input value is null and the field is not nullable, 
**  	    throw ERR_GC401A_CONVERSION_ERR.   
**	25-Aug-06 (gordy)
**	    Descriptor changed to hold mode flags.
*/

public void
updateBigDecimal( int index, BigDecimal value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log( title + ".updateBigDecimal(" + index + "," + value + ")" );
    
    index = columnMap( index );
    if ( value == null  &&  (rsmd.desc[ index ].flags & MSG_DSC_NULL) == 0 )
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
    initParam( index );
    params.setBigDecimal( index, value );
    return;
} // updateBigDecimal


/*
** Name: updateString
**
** Description:
**	Set column to a String value.
**
** Input:
**	index	Column index.
**	value	String value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-03 (gordy)
**	    Implemented.
**	23-Apr-04 (wansh01)
**	    if input value is null and the field is not nullable, 
**  	    throw ERR_GC401A_CONVERSION_ERR.   
**	25-Aug-06 (gordy)
**	    Descriptor changed to hold mode flags.
*/

public void
updateString( int index, String value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log( title + ".updateString(" + index + "," + value + ")" );
    
    index = columnMap( index );
    if ( value == null  &&  (rsmd.desc[ index ].flags & MSG_DSC_NULL) == 0 )
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
    initParam( index );
    params.setString( index, value );
    return;
} // updateString


/*
** Name: updateBytes
**
** Description:
**	Set column to a byte array value.
**
** Input:
**	index	Column index.
**	value	Byte array value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-03 (gordy)
**	    Implemented.
**	23-Apr-04 (wansh01)
**	    if input value is null and the field is not nullable, 
**  	    throw ERR_GC401A_CONVERSION_ERR.   
**	25-Aug-06 (gordy)
**	    Descriptor changed to hold mode flags.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Clean up tracing of array parameter.
*/

public void
updateBytes( int index, byte value[] ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log( title + ".updateBytes(" + index + "," + 
		   (value == null ? "null" : "[" + value.length + "]") + ")" );
	
    index = columnMap( index );
    if ( value == null  &&  (rsmd.desc[ index ].flags & MSG_DSC_NULL) == 0 )
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
    initParam( index );    
    params.setBytes( index, value );
    return;
} // updateBytes


/*
** Name: updateDate
**
** Description:
**	Set column to a Date value.
**
** Input:
**	index	Column index.
**	value	Date value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-03 (gordy)
**	    Implemented.
**	23-Apr-04 (wansh01)
**	    if input value is null and the field is not nullable, 
**  	    throw ERR_GC401A_CONVERSION_ERR.   
**	25-Aug-06 (gordy)
**	    Descriptor changed to hold mode flags.
*/

public void
updateDate( int index, Date value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log( title + ".updateDate(" + index + "," + value + ")" );
    
    index = columnMap( index );
    if ( value == null  &&  (rsmd.desc[ index ].flags & MSG_DSC_NULL) == 0 )
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
    initParam( index );
    params.setDate( index, value, null );
    return;
} // updateDate


/*
** Name: updateTime
**
** Description:
**	Set column to a Time value.
**
** Input:
**	index	Column index.
**	value	Time value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-03 (gordy)
**	    Implemented.
**	23-Apr-04 (wansh01)
**	    if input value is null and the field is not nullable, 
**  	    throw ERR_GC401A_CONVERSION_ERR.   
**	25-Aug-06 (gordy)
**	    Descriptor changed to hold mode flags.
*/

public void
updateTime( int index, Time value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log( title + ".updateTime(" + index + "," + value + ")" );
    
    index = columnMap( index );
    if ( value == null  &&  (rsmd.desc[ index ].flags & MSG_DSC_NULL) == 0 )
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
    initParam( index );
    params.setTime( index, value, null );
    return;
} // updateTime


/*
** Name: updateTimestamp
**
** Description:
**	Set column to a Timestamp value.
**
** Input:
**	index	Column index.
**	value	Timestamp value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-03 (gordy)
**	    Implemented.
**	23-Apr-04 (wansh01)
**	    if input value is null and the field is not nullable, 
**  	    throw ERR_GC401A_CONVERSION_ERR.   
**	25-Aug-06 (gordy)
**	    Descriptor changed to hold mode flags.
*/

public void
updateTimestamp( int index, Timestamp value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log( title + ".updateTimestamp(" + index + "," + value + ")" );
    
    index = columnMap( index );
    if ( value == null  &&  (rsmd.desc[ index ].flags & MSG_DSC_NULL) == 0 )
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
    initParam( index );
    params.setTimestamp(  index, value, null );
    return;
} // updateTimestamp


/*
** Name: updateBinaryStream
**
** Description:
**	Set column to a binary stream value.
**
** Input:
**	index	Column index.
**	value	Binary InputStream value.
**	length	Number of bytes (ignored).
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-03 (gordy)
**	    Implemented.
**	23-Apr-04 (wansh01)
**	    if input value is null and the field is not nullable, 
**  	    throw ERR_GC401A_CONVERSION_ERR.   
**	25-Aug-06 (gordy)
**	    Descriptor changed to hold mode flags.
*/

public void
updateBinaryStream( int index, InputStream value, long length ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log(title + ".updateBinaryStream(" + index + "," + length + ")");
    
    index = columnMap( index );
    if ( value == null  &&  (rsmd.desc[ index ].flags & MSG_DSC_NULL) == 0 )
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
    initParam( index );
    params.setBinaryStream( index, value );
    return;
} // updateBinaryStream


/*
** Name: updateAsciiStream
**
** Description:
**	Set column to a ASCII stream value.
**
** Input:
**	index	Column index.
**	value	ASCII InputStream value.
**	length	Number of ASCII characters (ignored).
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-03 (gordy)
**	    Implemented.
**	23-Apr-04 (wansh01)
**	    if input value is null and the field is not nullable, 
**  	    throw ERR_GC401A_CONVERSION_ERR.   
**	25-Aug-06 (gordy)
**	    Descriptor changed to hold mode flags.
*/

public void
updateAsciiStream( int index, InputStream value, long length ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log(title + ".updateAsciiStream(" + index + "," + length + ")");

    index = columnMap( index );
    if ( value == null  &&  (rsmd.desc[ index ].flags & MSG_DSC_NULL) == 0 )
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
    initParam( index );
    params.setAsciiStream( index, value );
    return;
} // updateAsciiStream


/*
** Name: updateCharacterStream
**
** Description:
**	Set column to a character stream value.
**
** Input:
**	index	Column index.
**	value	Character Reader value.
**	length	Number of characters (ignored).
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-03 (gordy)
**	    Implemented.
**	23-Apr-04 (wansh01)
**	    if input value is null and the field is not nullable, 
**  	    throw ERR_GC401A_CONVERSION_ERR.   
**	25-Aug-06 (gordy)
**	    Descriptor changed to hold mode flags.
*/

public void
updateCharacterStream( int index, Reader value, long length ) 
    throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".updateCharacterStream(" + 
				       index + "," + length + ")" );
    index = columnMap( index );
    if ( value == null  &&  (rsmd.desc[ index ].flags & MSG_DSC_NULL) == 0 )
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
    initParam( index );
    params.setCharacterStream( index, value );
    return;
} // updateCharacterStream


/*
** Name: updateBlob
**
** Description:
**	Set column to a Blob value.
**
** Input:
**	index	Column index.
**	value	Blob value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	15-Nov-06 (gordy)
**	    Implemented.
**	16-Mar-09 (gordy)
**	    For internal LOBs, initialize paramter by value.
**	    Otherwise, initialize by type.
*/

public void
updateBlob( int index, Blob value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log( title + ".updateBlob(" + index + "," + value + ")" ); 

    index = columnMap( index );
    if ( value == null  &&  (rsmd.desc[ index ].flags & MSG_DSC_NULL) == 0 )
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );

    switch( rsmd.desc[ index ].sql_type )
    {
    case Types.LONGVARBINARY :
    case Types.BLOB :
	/*
	** Because coercions involving locators are not directly
	** supported, the parameter handling is based on the
	** parameter type for these target column types.
	** In general, it is best to treat the target as a LOB
	** unless a valid locator is provided as the parameter.
	** Note: this can be done because updated values for 
	** these types are not copied back to result row.
	*/
	if ( value == null )
	{
	    initParam( index );
	    params.setNull( index );
	}
	else  if ( value instanceof JdbcBlob  &&
		   ((JdbcBlob)value).isValidLocator( conn ) )
	{
	    /*
	    ** The parameter is a valid locator and 
	    ** must be passed to the DBMS as such.
	    */
	    DrvLOB lob = ((JdbcBlob)value).getLOB();
	    params.init( index, lob );

	    if ( lob instanceof DrvBlob )
		params.setBlob( index, (DrvBlob)lob );
	    else	// Should not happen
		params.setBinaryStream( index, value.getBinaryStream() );
	}
	else
	{
	    /*
	    ** LOB data must be streamed back to DBMS.
	    */
	    initParam( index );
	    params.setBinaryStream( index, value.getBinaryStream() );
	}
	break;

    default :
	/*
	** In general, the following will result in a conversion
	** exception since coercion of BLOBs to other types is
	** usually not supported
	**
	** Calling the most specific setXXX() method ensures
	** the target type supports coercion of the parameter.
	** Using setBinaryStream() is preferred over setBlob()
	** because locators can only be coerced in the DBMS.
	*/
	initParam( index );

	if ( value == null )
	    params.setBinaryStream( index, null );
	else  if ( value instanceof JdbcBlob  &&
		   ((JdbcBlob)value).isValidLocator( conn ) )
	{
	    DrvLOB lob = ((JdbcBlob)value).getLOB();

	    if ( value instanceof DrvBlob )
		params.setBlob( index, (DrvBlob)lob );
	    else
		params.setBinaryStream( index, value.getBinaryStream() );
	}
	else
	    params.setBinaryStream( index, value.getBinaryStream() );
	break;
    }

    return;
} // updateBlob


/*
** Name: updateClob
**
** Description:
**	Set column to a Clob value.
**
** Input:
**	index	Column index.
**	value	Clob value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	15-Nov-06 (gordy)
**	    Implemented.
**	16-Mar-09 (gordy)
**	    For internal LOBs, initialize paramter by value.
**	    Otherwise, initialize by type.
*/

public void
updateClob( int index, Clob value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log( title + ".updateClob(" + index + "," + value + ")" ); 

    index = columnMap( index );
    if ( value == null  &&  (rsmd.desc[ index ].flags & MSG_DSC_NULL) == 0 )
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );

    switch( rsmd.desc[ index ].sql_type )
    {
    case Types.LONGVARCHAR :
    case Types.CLOB :
	/*
	** Because coercions involving locators are not directly
	** supported, the parameter handling is based on the 
	** parameter type for these target column types.
	** In general, it is best to treat the target as a LOB
	** unless a valid locator is provided as the parameter.
	** Note: this can be done because updated values for 
	** these types are not copied back to result row.
	*/
	if ( value == null )
	{
	    initParam( index );
	    params.setNull( index );
	}
	else  if ( value instanceof JdbcClob  &&
		   ((JdbcClob)value).isValidLocator( conn ) )
	{
	    /*
	    ** The parameter is a valid locator and must be
	    ** passed to the DBMS as such.
	    */
	    DrvLOB lob = ((JdbcClob)value).getLOB();
	    params.init( index, lob );

	    if ( lob instanceof DrvClob )
		params.setClob( index, (DrvClob)lob );
	    else  if ( lob instanceof DrvNlob )
		params.setClob( index, (DrvNlob)lob );
	    else	// Should not happen
		params.setCharacterStream( index, value.getCharacterStream() );
	}
	else
	{
	    /*
	    ** LOB data must be streamed back to DBMS.
	    */
	    initParam( index );
	    params.setCharacterStream( index, value.getCharacterStream() );
	}
	break;

    default :
	/*
	** In general, the following will result in a conversion
	** exception since coercion of CLOBs to other types is
	** usually not supported.
	**
	** Calling the most specific setXXX() method ensures
	** the target type supports coercion of the parameter.
	** Using setCharacterStream() is preferred over setClob()
	** because locators can only be coerced in the DBMS.
	*/
	initParam( index );

	if ( value == null )
	    params.setCharacterStream( index, null );
	else  if ( value instanceof JdbcClob  &&
		   ((JdbcClob)value).isValidLocator( conn ) )
	{
	    DrvLOB lob = ((JdbcClob)value).getLOB();

	    if ( lob instanceof DrvClob )
		params.setClob( index, (DrvClob)lob );
	    else  if ( lob instanceof DrvNlob )
		params.setClob( index, (DrvNlob)lob );
	    else	// Should not happen
		params.setCharacterStream( index, value.getCharacterStream() );
	}
	else
	    params.setCharacterStream( index, value.getCharacterStream() );
	break;
    }

    return;
} // updateClob


/*
** Name: updateObject
**
** Description:
**	Set column to a Java object value.
**
** Input:
**	index	Column index.
**	value	Java object value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-03 (gordy)
**	    Implemented.
**	23-Apr-04 (wansh01)
**	    if input value is null and the field is not nullable, 
**  	    throw ERR_GC401A_CONVERSION_ERR.   
**	25-Aug-06 (gordy)
**	    Descriptor changed to hold mode flags.
**	15-Nov-06 (gordy)
**	    Support BLOB/CLOB.
**	16-Mar-09 (gordy)
**	    For internal LOBs, initialize paramter by value.
**	    Otherwise, initialize by type.
*/

public void 
updateObject( int index, Object value ) 
    throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".updateObject(" + index + 
				       "," + value + ")" );
    index = columnMap( index );
    if ( value == null  &&  (rsmd.desc[ index ].flags & MSG_DSC_NULL) == 0 )
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );

    switch( rsmd.desc[ index ].sql_type )
    {
    case Types.LONGVARBINARY :
    case Types.BLOB :
	/*
	** Because coercions involving locators are not directly
	** supported, the parameter handling is based on the
	** parameter type for these target column types.
	** In general, it is best to treat the target as a LOB
	** unless a valid locator is provided as the parameter.
	** Note: this can be done because updated values for 
	** these types are not copied back to result row.
	*/
	if ( value == null  ||  ! (value instanceof Blob) )
	    initParam( index );
	else  if ( value instanceof JdbcBlob  &&
		   ((JdbcBlob)value).isValidLocator( conn ) )
	{
	    value = ((JdbcBlob)value).getLOB();
	    params.init( index, value );
	}
	else
	{
	    value = ((Blob)value).getBinaryStream();
	    initParam( index );
	}
	break;

    case Types.LONGVARCHAR :
    case Types.CLOB :
	/*
	** Because coercions involving locators are not directly
	** supported, the parameter handling is based on the 
	** parameter type for these target column types.
	** In general, it is best to treat the target as a LOB
	** unless a valid locator is provided as the parameter.
	** Note: this can be done because updated values for 
	** these types are not copied back to result row.
	*/
	if ( value == null  ||  ! (value instanceof Clob) )
	    initParam( index );
	else  if ( value instanceof JdbcClob  &&
		   ((JdbcClob)value).isValidLocator( conn ) )
	{
	    value = ((JdbcClob)value).getLOB();
	    params.init( index, value );
	}
	else
	{
	    value = ((Clob)value).getCharacterStream();
	    initParam( index );
	}
	break;

    default :
	/*
	** BLOB/CLOB values can represent both locators and
	** LOB data, so the underlying representation must
	** be extracted to be correctly assigned.
	*/
	if ( value != null )
	    if ( value instanceof Blob )
	    {
		if ( value instanceof JdbcBlob  &&
		     ((JdbcBlob)value).isValidLocator( conn ) )
		    value = ((JdbcBlob)value).getLOB();
		else
		    value = ((Blob)value).getBinaryStream();
	    }
	    else  if ( value instanceof Clob )
	    {
		if ( value instanceof JdbcClob  &&
		     ((JdbcClob)value).isValidLocator( conn ) )
		    value = ((JdbcClob)value).getLOB();
		else
		    value = ((Clob)value).getCharacterStream();
	    }

	initParam( index );
	break;
    }

    params.setObject( index, value );
    return;
} // updateObject


/*
** Name: updateObject
**
** Description:
**	Set column to a Java object value.
**
** Input:
**	index	Column index.
**	value	Java object value.
**	scale	Number of digits following decimal point, or -1.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-03 (gordy)
**	    Implemented.
**	23-Apr-04 (wansh01)
**	    if input value is null and the field is not nullable, 
**  	    throw ERR_GC401A_CONVERSION_ERR.   
**	28-Apr-04 (wansh01)
**	    Added type as an input for setObejct().
**	25-Aug-06 (gordy)
**	    Descriptor changed to hold mode flags.
**	15-Nov-06 (gordy)
**	    Support BLOB/CLOB.
**	16-Mar-09 (gordy)
**	    For internal LOBs, initialize paramter by value.
**	    Otherwise, initialize by type.
*/

public void 
updateObject( int index, Object value, int scale ) 
    throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".updateObject(" + index + 
				       "," + value + "," + scale + ")" );
    index = columnMap( index );
    if ( value == null  &&  (rsmd.desc[ index ].flags & MSG_DSC_NULL) == 0 )
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );

    int sqlType = rsmd.desc[ index ].sql_type;

    switch( sqlType )
    {
    case Types.LONGVARBINARY :
    case Types.BLOB :
	/*
	** Because coercions involving locators are not directly
	** supported, the parameter handling is based on the
	** parameter type for these target column types.
	** In general, it is best to treat the target as a LOB
	** unless a valid locator is provided as the parameter.
	** Note: this can be done because updated values for 
	** these types are not copied back to result row.
	*/

	if ( value == null  ||  ! (value instanceof Blob) )
	{
	    sqlType = Types.LONGVARBINARY;
	    initParam( index );
	}
	else  if ( value instanceof JdbcBlob  &&
		   ((JdbcBlob)value).isValidLocator( conn ) )
	{
	    sqlType = Types.BLOB;
	    value = ((JdbcBlob)value).getLOB();
	    params.init( index, value );
	}
	else
	{
	    sqlType = Types.LONGVARBINARY;
	    value = ((Blob)value).getBinaryStream();
	    initParam( index );
	}
	break;
 
    case Types.LONGVARCHAR :
    case Types.CLOB :
	/*
	** Because coercions involving locators are not directly
	** supported, the parameter handling is based on the 
	** parameter type for these target column types.
	** In general, it is best to treat the target as a LOB
	** unless a valid locator is provided as the parameter.
	** Note: this can be done because updated values for 
	** these types are not copied back to result row.
	*/
	if ( value == null  ||  ! (value instanceof Clob) )
	{
	    sqlType = Types.LONGVARCHAR;
	    initParam( index );
	}
	else  if ( value instanceof JdbcClob  &&
		   ((JdbcClob)value).isValidLocator( conn ) )
	{
	    sqlType = Types.CLOB;
	    value = ((JdbcClob)value).getLOB();
	    params.init( index, value );
	}
	else
	{
	    sqlType = Types.LONGVARCHAR;
	    value = ((Clob)value).getCharacterStream();
	    initParam( index );
	}
	break;
 
    default :
	/*
	** BLOB/CLOB values can represent both locators and
	** LOB data, so the underlying representation must
	** be extracted to be correctly assigned.
	*/
	if ( value != null )
	    if ( value instanceof Blob )
	    {
		if ( value instanceof JdbcBlob  &&
		     ((JdbcBlob)value).isValidLocator( conn ) )
		    value = ((JdbcBlob)value).getLOB();
		else
		    value = ((Blob)value).getBinaryStream();
	    }
	    else  if ( value instanceof Clob )
	    {
		if ( value instanceof JdbcClob  &&
		     ((JdbcClob)value).isValidLocator( conn ) )
		    value = ((JdbcClob)value).getLOB();
		else
		    value = ((Clob)value).getCharacterStream();
	    }

	initParam( index );
	break;
    }

    params.setObject( index, value, sqlType, scale );
    return;
} // updateObject


/*
** Name: deleteRow
**
** Description:
**	Delete current row.
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
**	 1-Nov-03 (gordy)
**	    Implemented.
*/

public synchronized void 
deleteRow() 
    throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".deleteRow()" );
    
    if ( insert  ||  posStatus != ROW  ||  
	 (currentRow.status & Row.DELETED) != 0 )
    {
	if ( trace.enabled() )  trace.log(title + ".deleteRow(): invalid row");
	throw SqlExFactory.get( ERR_GC4021_INVALID_ROW ); 
    }
    
    try
    {
	/*
	** Active BLOB streams must be flushed.
	*/
	flush();
	exec( MSG_QRY_CDEL, "delete from " + table, null );
	currentRow.status |= Row.DELETED;
	currentRow.status &= ~Row.UPDATED;	// Updates no longer relevant 
    }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ".deleteRow(): error deleting row" );
	    SqlExFactory.trace( ex, trace );
	}
	
	throw ex;
    }
    finally
    {
	params.clear( false ); 
    }
    
    if ( trace.enabled() )  trace.log( title + ".deleteRow(): row deleted!" );
    return;
} // deleteRow


/*
** Name: updateRow
**
** Description:
**	Update current row.
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
**	 1-Nov-03 (gordy)
**	    Implemented.
*/

public synchronized void 
updateRow() throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".updateRow()" );
    
    if ( insert  ||  posStatus != ROW  ||  
	 (currentRow.status & Row.DELETED) != 0 )
    {
	if ( trace.enabled() )  trace.log(title + ".updateRow(): invalid row");
	throw SqlExFactory.get( ERR_GC4021_INVALID_ROW ); 
    }

    /*
    ** Build list of columns to be updated.
    */
    StringBuffer sb = new StringBuffer();
    
    for( int param = 0; param < rsmd.count; param++ )
	if ( params.isSet( param ) )
	{
	    if ( sb.length() > 0 )  sb.append( ", " );
	    sb.append( rsmd.desc[ param ].name );
	    sb.append( " = ~V " );
	}
    
    /*
    ** If no columns/parameters have been set, quietly ignore request.
    */
    if ( sb.length() <= 0 )
    {
	if ( trace.enabled() )  trace.log( title + 
			".updateRow(): no columns updated, request ignored." );
	return;
    }
    
    String query = "update " + table + " set " + sb.toString();
	
    try
    {
	/*
	** Active BLOB streams must be flushed.
	*/
	flush();
	exec( MSG_QRY_CUPD, query, params );
	currentRow.status |= Row.UPDATED;
	// TODO: copy column values to current row
    }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ".updateRow(): error updating row" );
	    SqlExFactory.trace( ex, trace );
	}
	
	throw ex;
    }
    finally 
    { 
	copyParams();
	params.clear( false ); 
    }
    
    if ( trace.enabled() ) trace.log(title + ".updateRow(): row updated!");
    return;
} // updateRow


/*
** Name: insertRow
**
** Description:
**	Insert new row.
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
**	 1-Nov-03 (gordy)
**	    Implemented.
**	 6-Apr-07 (gordy)
**	    Current row flushed when moved to insert row.
*/

public synchronized void 
insertRow() 
    throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".insertRow()" );
    
    if ( ! insert  ||  posStatus == CLOSED )
    {
	if ( trace.enabled() )  trace.log(title + ".insertRow(): invalid row");
	throw SqlExFactory.get( ERR_GC4021_INVALID_ROW ); 
    }

    /*
    ** Build list of columns/values to be updated.
    */
    StringBuffer sb1 = new StringBuffer();
    StringBuffer sb2 = new StringBuffer();
    
    for( int param = 0; param < rsmd.count; param++ )
	if ( params.isSet( param ) )
	{
	    if ( sb1.length() > 0 )  
	    {
		sb1.append( "," );
		sb2.append( "," );
	    }
	    
	    sb1.append( rsmd.desc[ param ].name );
	    sb2.append( " ~V " );
	}
    
    /*
    ** Can't insert if no columns/parameters have been set.
    */
    if ( sb1.length() <= 0 )
    {
	if ( trace.enabled() )  trace.log( title + ".insertRow(): no values" );
	throw SqlExFactory.get( ERR_GC4020_NO_PARAM ); 
    }
    
    String query = "insert into " + table + "(" + sb1.toString() + 
		   ") values(" + sb2.toString() + ")";
	
    try
    {
	exec( MSG_QRY_EXQ, query, params );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ".insertRow(): error inserting row" );
	    SqlExFactory.trace( ex, trace );
	}
	
	throw ex;
    }
    finally 
    { 
	params.clear( false ); 
    }
    
    if ( trace.enabled() ) trace.log( title + ".insertRow(): row inserted!" );
    return;
} // insertRow


/*
** Name: exec
**
** Description:
**	Executes a statement.  If the statement type is cursor
**	update or delete, the cursor name is sent with request.
**	If a parameter set is provided, descriptor and data
**	messages are also sent.
**
** Input:
**	type		Query type: MSG_QRY_{CDEL,CUPD}
**	sql		Statement text.
**	param_set	Parameters (may be NULL).
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-03 (gordy)
**	    Created.
**	 9-Nov-06 (gordy)
**	    Added writeQueryText() to segment query text.
*/

private void
exec( short type, String sql, ParamSet param_set )
    throws SQLException
{
    clearResults();
    msg.lock();
    
    try
    {
	msg.begin( MSG_QUERY );
	msg.write( type );
    
	if ( type == MSG_QRY_CDEL  ||  type == MSG_QRY_CUPD )
	{
	    msg.write( MSG_QP_CRSR_NAME );
	    msg.write( cursor );
	}
    
	writeQueryText( sql );
	
	if ( param_set == null )
	    msg.done( true );
	else
	{
	    msg.done( false );
	    param_set.sendDesc( false );
	    param_set.sendData( true );
	}
	
	readResults();
    }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ": error executing query: " + sql );
	    SqlExFactory.trace( ex, trace );
	}
	
	throw ex;
    }
    finally
    {
	msg.unlock();
    }
    
    return;
} // exec


/*
** Name: initParam
**
** Description:
**	Initialize a parameter based on the associated column type.
**	Parameter value is not definitively initialized: a param-set
**	setXXX() method must be called to set the parameter value.
**
**	If the associated column type is a LOB Locator (BLOB or CLOB),
**	the parameter type is forced to the appropriate LOB type since
**	this will be most useful in general.  If Locators types are
**	actually needed, the parameter should be initialized directly.
**
** Input:
**	index	Internal parameter index [0,rsmd.count-1].
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-03 (gordy)
**	    Created.
**	23-Apr-04 (wansh01)
**	    Moved columnMap() to individual rtns which calls initParms. 
**	19-Jun-06 (gordy)
**	    ANSI Date/Time data type support.
**	26-Feb-07 (gordy)
**	    Changed to void since return value no longer useful.
**	    Parameter types now forced to LOB for Locators.
*/

private synchronized void
initParam( int index )
    throws SQLException
{
    int		type = rsmd.desc[ index ].sql_type;
    boolean	alt = false;
    
    switch( rsmd.desc[ index ].sql_type )
    {
    case Types.CHAR :
        alt = (rsmd.desc[ index ].dbms_type != DBMS_TYPE_NCHAR);
	break;

    case Types.VARCHAR :    
        alt = (rsmd.desc[ index ].dbms_type != DBMS_TYPE_NVARCHAR);
	break;

    case Types.LONGVARCHAR :
        alt = (rsmd.desc[ index ].dbms_type != DBMS_TYPE_LONG_NCHAR);
	break;

    case Types.BLOB :
	type = Types.LONGVARBINARY;
	break;

    case Types.CLOB :
	type = Types.LONGVARCHAR;
	alt = (rsmd.desc[ index ].dbms_type != DBMS_TYPE_LNLOC);
	break;

    case Types.TIMESTAMP :
	alt = (rsmd.desc[ index ].dbms_type == DBMS_TYPE_IDATE);
	break;
    }
    
    params.init( index, type, alt );
    return;
} // initParam


/*
** Name: copyParams
**
** Description:
**	Copies parameter values (if set) to the current row
**	column data values.
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
**	 1-Dec-03 (gordy)
**	    Created.
**	15-Mar-04 (gordy)
**	    Support BIGINT columns.
**	19-Jun-06 (gordy)
**	    ANSI Date/Time data type support.
**	15-Nov-06 (gordy)
**	    Support BLOB/CLOB.
**	26-Oct-09 (gordy)
**	    Support BOOLEAN columns.
*/

private void
copyParams()
    throws SQLException
{
    for( int i = 0; i < rsmd.count; i++ )
    {
	if ( ! params.isSet( i ) )  continue;
	SqlData data = currentRow.columns[ i ];
	
        switch( rsmd.desc[ i ].sql_type )
        {
	case Types.NULL :	break;	// Nothing to do.
	    
        case Types.TINYINT : 
	    ((SqlTinyInt)data).set( (SqlTinyInt)params.getValue( i ) );
    	    break;

        case Types.SMALLINT :	
	    ((SqlSmallInt)data).set( (SqlSmallInt)params.getValue(i) );
    	    break;

        case Types.INTEGER : 
	    ((SqlInt)data).set( (SqlInt)params.getValue( i ) );
    	    break;

        case Types.BIGINT : 
	    ((SqlBigInt)data).set( (SqlBigInt)params.getValue( i ) );
    	    break;

        case Types.REAL : 
	    ((SqlReal)data).set( (SqlReal)params.getValue( i ) );
    	    break;

        case Types.FLOAT :
        case Types.DOUBLE :	
	    ((SqlDouble)data).set( (SqlDouble)params.getValue( i ) );
    	    break;

        case Types.NUMERIC :
        case Types.DECIMAL : 
	    ((SqlDecimal)data).set( (SqlDecimal)params.getValue( i ) );
	    break;

	case Types.BOOLEAN :
	    ((SqlBool)data).set( (SqlBool)params.getValue( i ) );
	    break;

        case Types.DATE :
	    ((SqlDate)data).set( (SqlDate)params.getValue( i ) );
	    break;

	case Types.TIME :
	    ((SqlTime)data).set( (SqlTime)params.getValue( i ) );
	    break;

        case Types.TIMESTAMP :
	    switch( rsmd.desc[ i ].dbms_type )
	    {
	    case DBMS_TYPE_IDATE :
		((IngresDate)data).set( (IngresDate)params.getValue( i ) );
		break;

	    default :
		((SqlTimestamp)data).set( (SqlTimestamp)params.getValue( i ) );
		break;
	    }
    	    break;

        case Types.BINARY :
	    ((SqlByte)data).set( (SqlByte)params.getValue( i ) );
    	    break;

        case Types.VARBINARY :  
	    ((SqlVarByte)data).set( (SqlVarByte)params.getValue( i ) );
    	    break;

        case Types.CHAR :
        case Types.NCHAR :
    	    if ( rsmd.desc[ i ].dbms_type != DBMS_TYPE_NCHAR )
		((SqlChar)data).set( (SqlChar)params.getValue( i ) );
    	    else
		((SqlNChar)data).set( (SqlNChar)params.getValue( i ) );
    	    break;

        case Types.VARCHAR :    
        case Types.NVARCHAR :    
    	    if ( rsmd.desc[ i ].dbms_type != DBMS_TYPE_NVARCHAR )
		((SqlVarChar)data).set( (SqlVarChar)params.getValue(i) );
    	    else
		((SqlNVarChar)data).set((SqlNVarChar)params.getValue(i));
    	    break;

        case Types.LONGVARBINARY :
        case Types.LONGVARCHAR :
        case Types.LONGNVARCHAR :
	    /*
	    ** A BLOB may only be read once, and the parameter
	    ** should be read to send the BLOB to the DBMS.  The
	    ** column must have been consumed prior to any update.
	    ** There is nothing that can be copied.  In addition,
	    ** the parameter may have been a locator, in which
	    ** case the type will not match the column.
	    */
	    break;

	case Types.BLOB :
	case Types.CLOB :
	    /*
	    ** While locators can be copied, they don't support
	    ** coercions with streams and string/binary values.
	    ** Support for these other types are supported here
	    ** by setting the value type based on the parameter
	    ** rather than the associated column.  Because the
	    ** parameter type may not match the column type, the
	    ** parameter value is not copied to the column value.
	    */
	    break;

        default :
    	    if ( trace.enabled( 1 ) )
    	        trace.write( tr_id + ": unexpected SQL type " + 
    	    		     rsmd.desc[ i ].sql_type );
    	    throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
        }
    }

    return;
} // copyParams


/*
** Name: columnMap
**
** Description:
**	Determines if a given column index is a part of the result
**	set and maps the external index to the internal index.
**
**	Overrides super-class to handle insert row case.
**
** Input:
**	index	External column index [1,rsmd.count].
**
** Output:
**	None.
**
** Returns:
**	int	Internal column index [0,rsmd.count - 1].
**
** History:
**	 1-Nov-03 (gordy)
**	    Created.
*/

protected int
columnMap( int index )
    throws SQLException
{
    /*
    ** Super-class handles current row access.
    */
    if ( ! insert )  return( super.columnMap( index ) );

    if ( posStatus == CLOSED ) 
        throw SqlExFactory.get( ERR_GC401D_RESULTSET_CLOSED );
	
    if ( index < 1  ||  index > rsmd.count )
        throw SqlExFactory.get( ERR_GC4011_INDEX_RANGE );

    return( index - 1 );	// Internally, column indexes start at 0.
} // columnMap


/*
** Name: getSqlData
**
** Description:
**	Returns the SqlData object for a column.
**
** Input:
**	index	Internal column index [0,rsmd.count - 1].
**
** Output:
**	None.
**
** Returns:
**	SqlData	Column data value.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

protected SqlData
getSqlData( int index )
    throws SQLException
{
    SqlData data;

    /*
    ** If a column value has been updated (parameter set),
    ** then the updated data value is returned.  The insert 
    ** row requires the column to have been updated, other-
    ** wise the super-class is called to obtain the result
    ** data value.
    */
    if ( params.isSet( index ) )
	data = params.getValue( index );
    else  if ( ! insert )
	data = super.getSqlData( index );
    else
	throw SqlExFactory.get( ERR_GC4011_INDEX_RANGE );

    return( data );
} // getSqlData

/*
** Update methods which are simple covers to the
** equivalent methods implemented in this class
*/

public void
updateNClob(int columnIndex, NClob nclob)
throws SQLException
{ updateClob( columnIndex, (Clob) nclob ); }

public void
updateNClob(int columnIndex, Reader reader)
throws SQLException
{ updateCharacterStream( columnIndex, reader, -1L ); }

public void
updateNClob(int columnIndex, Reader reader, long length)
throws SQLException
{ updateCharacterStream( columnIndex, reader, length ); } 

public void
updateNString(int columnIdx, String nstring)
throws SQLException
{ updateString( columnIdx, nstring ); }

public void
updateClob(int columnIndex, Reader reader)
throws SQLException
{ updateCharacterStream( columnIndex, reader, -1L ); }

public void
updateClob(int columnIndex, Reader reader, long length)
throws SQLException
{ updateCharacterStream( columnIndex, reader, length ); } 

public void
updateBlob(int columnIndex, InputStream stream )
throws SQLException
{ updateBinaryStream( columnIndex, stream, -1L ); }

public void
updateBlob(int columnIndex, InputStream inputStream, long length)
throws SQLException
{ updateBinaryStream( columnIndex, inputStream, length ); } 

public void
updateCharacterStream(int columnIndex, Reader reader)
throws SQLException
{ updateCharacterStream( columnIndex, reader, -1L ); }

public void
updateCharacterStream(int columnIndex, Reader reader, int length)
throws SQLException
{ updateCharacterStream( columnIndex, reader, (long)length ); } 

public void
updateNCharacterStream(int columnIndex, Reader reader)
throws SQLException
{ updateCharacterStream( columnIndex, reader, -1L ); } 

public void
updateNCharacterStream(int columnIndex, Reader reader, long length)
throws SQLException
{ updateCharacterStream( columnIndex, reader, length ); } 

public void
updateBinaryStream(int columnIndex, InputStream  stream )
throws SQLException
{ updateBinaryStream( columnIndex, stream, -1L ); }

public void
updateBinaryStream(int columnIndex, InputStream stream, int length)
throws SQLException
{ updateBinaryStream( columnIndex, stream, (long)length ); } 

public void
updateAsciiStream(int columnIndex, InputStream stream )
throws SQLException
{ updateAsciiStream( columnIndex, stream, -1L ); }

public void
updateAsciiStream(int columnIndex, InputStream stream, int length)
throws SQLException
{ updateAsciiStream( columnIndex, stream, (long)length ); }


} // class RsltUpd
