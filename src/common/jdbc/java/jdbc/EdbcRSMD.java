/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbc;

/*
** Name: EdbcRSMD
**
** Description:
**	Implements the EDBC JDBC ResultSetMetaData class: EdbcRSMD.
**
**  Classes:
**
**	EdbcRSMD
**	Desc
**
** History:
**	17-May-99 (gordy)
**	    Created.
**	13-Sep-99 (gordy)
**	    Implemented error code support.
**	18-Nov-99 (gordy)
**	    Made Desc a nested class.
**	19-Nov-99 (gordy)
**	    Completed stubbed methods.
**	25-Aug-00 (gordy)
**	    Nullable byte converted to flags.
**	 4-Oct-00 (gordy)
**	    Standardized internal tracing.
**	10-Nov-00 (gordy)
**	    Added support for JDBC 2.0 extensions.
**	28-Mar-01 (gordy)
**	    Tracing addes as constructor parameter.
**	16-Apr-01 (gordy)
**	    Convert constructor to factory method and add companion
**	    methods to permit optimization of RSMD handling.
**	10-May-01 (gordy)
**	    Added support for UCS2 datatypes.
*/

import	java.math.BigDecimal;
import	java.sql.ResultSetMetaData;
import	java.sql.Date;
import	java.sql.Time;
import	java.sql.Timestamp;
import	java.sql.Types;
import	java.sql.SQLException;
import	ca.edbc.util.DbmsConst;
import	ca.edbc.util.MsgConst;
import	ca.edbc.util.EdbcEx;
import	ca.edbc.util.IdMap;
import	ca.edbc.io.DbConn;


/*
** Name: EdbcRSMD
**
** Description:
**	EDBC JDBC Java driver class which implements the
**	JDBC ResultSetMetaData interface.
**
**  Interface Methods:
**
**	getColumnCount	    Returns the number of columns in the result set.
**	isAutoIncrement	    Determine if column is automatically numbered.
**	isCaseSensitive	    Determine if column is case sensitive.
**	isSearchable	    Determine if column may be used in a where clause.
**	isCurrency	    Determine if column is a cash value.
**	isNullable	    Determine if column is nullable.
**	isSigned	    Determine if column is a signed number.
**	getColumnDisplaySize	Returns the display width of a column.
**	getColumnLabel	    Returns the title for a column.
**	getColumnName	    Returns the name for a column.
**	getSchemaName	    Returns the schema name in which the column resides.
**	getPrecision	    Returns the number of digits for a numeric column.
**	getScale	    Returns the number of digits following decimal point.
**	getTableName	    Returns the table name in which the column resides.
**	getCatalogName	    Returns the catalog name in which the column resides.
**	getColumnType	    Returns the SQL type for a column.
**	getColumnTypeName   Returns the host DBMS type for a column.
**	isReadOnly	    Determine if a column is read-only.
**	isWritable	    Determine if a column may be updated.
**	isDefinitelyWritable	Determine if a column update will always succeed.
**	getColumnClassName  Returns the class name of object from getObject().
**
**
**  Public Data:
**
**	count		    Number of columns in the result set.
**	desc		    Column descriptors.
**
**  Public Methods:
**
**	load		    Read descriptor from connection, generate RSMD.
**	reload		    Read descriptor from connection, update RSMD.
**	resize		    Set number of column descriptors.
**	setColumnInfo	    Set Column Info.
**
**  Private Data:
**
**	trace		    Tracing.
**	title		    Class title for tracing.
**	tr_id		    ID for tracing.
**	inst_count	    Number of instances.
**
**  Private Methods:
**
**	columnMap	    Map an extern column index to the internal index.
**
** History:
**	17-May-99 (gordy)
**	    Created.
**	18-Nov-99 (gordy)
**	    Made Desc a nested class.
**	 4-Oct-00 (gordy)
**	    Added instance count, inst_count, for standardized internal tracing.
**	10-Nov-00 (gordy)
**	    Support JDBC 2.0 extensions.  Added getColumnClassName().
**	16-Apr-01 (gordy)
**	    Converted constructor with DbConn to load() factory method
**	    and added companion reload() method to re-read Result-set
**	    Meta-data into existing RSMD.  A re-read should not change
**	    the number of columns, but added resize() method to be safe.
*/

class
EdbcRSMD
    implements	ResultSetMetaData, MsgConst, DbmsConst, EdbcConst, EdbcErr
{

    /*
    ** The following members are declared with default access
    ** to provide easy access to the meta-data to other parts
    ** of the driver.
    */
    short   count = 0;		// Number of descriptors.
    Desc    desc[];		// Descriptors.

    private EdbcTrace		trace = null;
    private String		title = null;
    private String		tr_id = null;
    private static int		inst_count = 0;


/*
** Name: EdbcRSMD
**
** Description:
**	Class constructor.  Allocates descriptors as requested
**	by the caller.  It is the callers responsibility to
**	populate the descriptor information.
**	
**
** Input:
**	count	Number of descriptors.
**	trace	Connection tracing.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	17-May-99 (gordy)
**	    Created.
**	28-Mar-01 (gordy)
**	    Tracing added as a parameter.
*/

public 
EdbcRSMD( short count, EdbcTrace trace )
{
    title = shortTitle + "-RSMD[" + inst_count + "]";
    tr_id = "RSMD[" + inst_count + "]";
    inst_count++;
    this.trace = trace;
    
    this.count = count;
    desc = new Desc[ count ];
    for( int col = 0; col < count; col++ )  desc[ col ] = new Desc();
} // EdbcRSMD


/*
** Name: toString
**
** Description:
**	Return the formatted name of this object.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	    Object name.
**
** History:
**	 7-Nov-00 (gordy)
**	    Created.
*/

public String
toString()
{
    return( title );
}


/*
** Name: getColumnCount
**
** Description:
**	Returns the number of columns in the associated result set.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	ResultSet   A new ResultSet object.
**
** History:
**	17-May-99 (gordy)
**	    Created.
*/

public int
getColumnCount()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getColumnCount: " + count );
    return( count );
} // getColumnCount

/*
** Name: getColumnName
**
** Description:
**	Returns the name of the column.
**
** Input:
**	column	    Column index.
**
** Output:
**	None.
**
** Returns:
**	String	    Column name.
**
** History:
**	17-May-99 (gordy)
**	    Created.
*/

public String
getColumnName( int column )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getColumnName( " + column + " )" );
    column = columnMap( column );
    if ( trace.enabled() )  
	trace.log( title + ".getColumnName: " + desc[ column ].name );
    return( desc[ column ].name );
} // getColumnName

/*
** Name: getColumnLabel
**
** Description:
**	Returns the title of the column.
**
** Input:
**	column	    Column index.
**
** Output:
**	None.
**
** Returns:
**	String	    Column title.
**
** History:
**	17-May-99 (gordy)
**	    Created.
*/

public String
getColumnLabel( int column )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getColumnLabel( " + column + " )" );
    column = columnMap( column );
    if ( trace.enabled() )  
	trace.log( title + ".getColumnLabel: " + desc[ column ].name );
    return( desc[ column ].name );
} // getColumnLabel

/*
** Name: getTableName
**
** Description:
**	Returns the name of the table in which the column resides.
**
** Input:
**	column	    Column index.
**
** Output:
**	None.
**
** Returns:
**	String	    Table name.
**
** History:
**	17-May-99 (gordy)
**	    Created.
*/

public String
getTableName( int column )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getTableName( " + column + " ): ''" );
    column = columnMap( column );
    return( "" );
} // getTableName

/*
** Name: getSchemaName
**
** Description:
**	Returns the name of the schema in which the column resides.
**
** Input:
**	column	    Column index.
**
** Output:
**	None.
**
** Returns:
**	String	    Schema name.
**
** History:
**	17-May-99 (gordy)
**	    Created.
*/

public String
getSchemaName( int column )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getSchemaName( " + column + " ): ''" );
    column = columnMap( column );
    return( "" );
} // getSchemaName

/*
** Name: getCatalogName
**
** Description:
**	Returns the name of the catalog in which the column resides.
**
** Input:
**	column	    Column index.
**
** Output:
**	None.
**
** Returns:
**	String	    Catalog name.
**
** History:
**	17-May-99 (gordy)
**	    Created.
*/

public String
getCatalogName( int column )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getCatalogName( " + column + " ): ''" );
    column = columnMap( column );
    return( "" );
} // getCatalogName

/*
** Name: getColumnType
**
** Description:
**	Execute an SQL statement and return an indication as to the
**	type of the result.
**
** Input:
**	column	    Column index
**
** Output:
**	None.
**
** Returns:
**	int	    Column SQL type (java.sql.Types).
**
** History:
**	17-May-99 (gordy)
**	    Created.
*/

public int
getColumnType( int column )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getColumnType( " + column + " )" );
    column = columnMap( column );
    if ( trace.enabled() )  
	trace.log( title + ".getColumnType: " + desc[ column ].sql_type );
    return( desc[ column ].sql_type );
} // getColumnType

/*
** Name: getColumnTypeName
**
** Description:
**	Returns the host DBMS type for a column.
**
** Input:
**	column	    Column index.
**
** Output:
**	None.
**
** Returns:
**	String	    DBMS type.
**
** History:
**	17-May-99 (gordy)
**	    Created.
*/

public String
getColumnTypeName( int column )
    throws SQLException
{
    String type = null;

    if ( trace.enabled() )  
	trace.log( title + ".getColumnTypeName( " + column + " )" );
    column = columnMap( column );
    
    switch( desc[ column ].dbms_type )
    {
    case DBMS_TYPE_INT :    
	type = IdMap.get( desc[ column ].length, intMap );
	break;

    case DBMS_TYPE_FLOAT :  
	type = IdMap.get( desc[ column ].length, floatMap );
	break;

    default :		    
	type = IdMap.get( desc[ column ].dbms_type, typeMap );
	break;
    }

    if ( trace.enabled() )  trace.log( title + ".getColumnTypeName: " + type );
    return( type );
} // getColumnTypeName


/*
** Name: getColumnClassName
**
** Description:
**	Returns the Java class name of the object returned by
**	getObject() for a column.
**
** Input:
**	column	    Column index.
**
** Output:
**	None.
**
** Returns:
**	String	    Class name.
**
** History:
**	10-Nov-00 (gordy)
**	    Created.
*/

public String
getColumnClassName( int column )
    throws SQLException
{
    String  name;
    Class   oc = Object.class;

    if ( trace.enabled() )  
	trace.log( title + ".getColumnTypeName( " + column + " )" );
    
    switch( desc[ (column = columnMap( column )) ].sql_type )
    {
    case Types.BIT :		oc = Boolean.class;	break;

    case Types.TINYINT :
    case Types.SMALLINT :
    case Types.INTEGER :	oc = Integer.class;	break;

    case Types.BIGINT :		oc = Long.class;	break;

    case Types.REAL :		oc = Float.class;	break;

    case Types.FLOAT :
    case Types.DOUBLE :		oc = Double.class;	break;

    case Types.NUMERIC :
    case Types.DECIMAL :	oc = BigDecimal.class;	break;

    case Types.CHAR :
    case Types.VARCHAR :
    case Types.LONGVARCHAR:	oc = String.class;	break;

    case Types.BINARY :
    case Types.VARBINARY :
    case Types.LONGVARBINARY :	oc = Byte.class;	break;

    case Types.DATE :		oc = Date.class;	break;
    case Types.TIME :		oc = Time.class;	break;
    case Types.TIMESTAMP :	oc = Timestamp.class;	break;

    default :		    
	if ( trace.enabled( 1 ) )
	    trace.write(title + ": invalid SQL type " + desc[column].sql_type);
	throw EdbcEx.get( E_JD0002_PROTOCOL_ERR );
    }

    name = oc.getName();
    if ( trace.enabled() )  trace.log( title + ".getColumnClassName: " + name );
    return( name );
} // getColumnClassName

/*
** Name: getPrecision
**
** Description:
**	Returns the number of decimal digits in a numeric column.
**
** Input:
**	column	    Column index.
**
** Output:
**	None.
**
** Returns:
**	int	    Number of numeric digits.
**
** History:
**	17-May-99 (gordy)
**	    Created.
**	25-Jan-00 (rajus01)
**	    Added CHAR,VARCHAR,BINARY,VARBINARY case.
**	10-May-01 (gordy)
**	    Added support for UCS2 datatypes.
*/

public int
getPrecision( int column )
    throws SQLException
{
    if ( trace.enabled() )  trace.log(title + ".getPrecision( " + column + " )");
    column = columnMap( column );
    int precision = 0;

    switch( desc[ column ].sql_type )
    {
    case Types.TINYINT :	precision = 3;	break;
    case Types.SMALLINT :	precision = 5;	break;
    case Types.INTEGER :	precision = 10;	break;
    case Types.BIGINT :		precision = 19;	break;
    case Types.REAL :		precision = 7;	break;
    case Types.FLOAT :
    case Types.DOUBLE :		precision = 15;	break;
    case Types.NUMERIC :
    case Types.DECIMAL :	precision = desc[ column ].precision;	break;
    case Types.BINARY :
    case Types.VARBINARY :	precision = desc[ column ].length;	break;
    
    case Types.CHAR :
	precision = desc[ column ].length;
	if ( desc[ column ].dbms_type == DBMS_TYPE_NCHAR )  precision /= 2;
	break;

    case Types.VARCHAR :
	precision = desc[ column ].length;
	if ( desc[ column ].dbms_type == DBMS_TYPE_NVARCHAR )  precision /= 2;
	break;
    }
    
    if ( trace.enabled() )  trace.log( title + ".getPrecision: " + precision );
    return( precision );
} // getPrecision

/*
** Name: getScale
**
** Description:
**	Returns the number of numeric digits following the decimal point.
**
** Input:
**	column	    Column index.
**
** Output:
**	None.
**
** Returns:
**	int	    Number of scale digits.
**
** History:
**	17-May-99 (gordy)
**	    Created.
*/

public int
getScale( int column )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getScale( " + column + " )" );
    column = columnMap( column );
    if ( trace.enabled() )  trace.log( title + ".getScale: " + desc[ column ].scale );
    return( desc[ column ].scale );
} // getScale

/*
** Name: getColumnDisplaySize
**
** Description:
**	Returns the display width of a column.
**
** Input:
**	column	    Column index.
**
** Output:
**	None.
**
** Returns:
**	int	    Column display width.
**
** History:
**	17-May-99 (gordy)
**	    Created.
**	10-May-01 (gordy)
**	    Added support for UCS2 datatypes.
*/

public int
getColumnDisplaySize( int column )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getColumnDisplaySize( " + column + " )" );
    column = columnMap( column );
    int len = 0;

    switch( desc[ column ].sql_type )
    {
    case Types.BIT :		len = 5;	break;
    case Types.TINYINT :	len = 4;	break;
    case Types.SMALLINT :	len = 6;	break;
    case Types.INTEGER :	len = 11;	break;
    case Types.BIGINT :		len = 20;	break;
    case Types.REAL :		len = 14;	break;
    case Types.FLOAT :
    case Types.DOUBLE :		len = 24;	break;
    case Types.DATE :		len = 10;	break;
    case Types.TIME :		len = 8;	break;
    case Types.TIMESTAMP :	len = 26;	break;
    case Types.BINARY :
    case Types.VARBINARY :	len = desc[ column ].length;	break;
    case Types.LONGVARCHAR :
    case Types.LONGVARBINARY :	len = 0;	break;	// what should this be?
    
    case Types.NUMERIC :
    case Types.DECIMAL :
	len = desc[ column ].precision + 1;
	if ( desc[ column ].scale > 0 )  len++;
	break;

    case Types.CHAR :
	len = desc[ column ].length;
	if ( desc[ column ].dbms_type == DBMS_TYPE_NCHAR )  len /= 2;
	break;

    case Types.VARCHAR :
	len = desc[ column ].length;
	if ( desc[ column ].dbms_type == DBMS_TYPE_NVARCHAR )  len /= 2;
	break;
    }

    if ( trace.enabled() )  trace.log( title + ".getColumnDisplaySize: " + len );
    return( len );
}

/*
** Name: isNullable
**
** Description:
**	Determine if column is nullable.  Returns one of the following:
**
**	    columnNoNulls
**	    columnNullable
**	    columnNullableUnknown
**
** Input:
**	column	    Column index.
**
** Output:
**	None.
**
** Returns:
**	int	    Column nullability.
**
** History:
**	17-May-99 (gordy)
**	    Created.
*/

public int
isNullable( int column )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".isNullable( " + column + " )" );
    column = columnMap( column );
    int nulls = desc[ column ].nullable ? columnNullable : columnNoNulls;
    if ( trace.enabled() )  
	trace.log( title + ".isNullable: " + 
		   (desc[column].nullable ? "columnNullable" : "columnNoNulls") );
    return( nulls );
} // isNullable

/*
** Name: isSearchable
**
** Description:
**	Determine if column may be used in a where clause.
**
** Input:
**	column	    Column index.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if column is searchable, false otherwise.
**
** History:
**	17-May-99 (gordy)
**	    Created.
*/

public boolean
isSearchable( int column )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".isSearchable( " + column + " ): " + true );
    column = columnMap( column );
    return( true );
} // isSearchable

/*
** Name: isCaseSensitive
**
** Description:
**	Determine if column is case sensitive.
**
** Input:
**	column	    Column index.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if column is case sensitive, false otherwise.
**
** History:
**	17-May-99 (gordy)
**	    Created.
*/

public boolean
isCaseSensitive( int column )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".isCaseSensitive( " + column + " ): " + true );
    column = columnMap( column );
    boolean sensitive = false;

    switch( desc[ column ].sql_type )
    {
    case Types.CHAR :
    case Types.VARCHAR :
    case Types.LONGVARCHAR :
	sensitive = true;
	break;
    }

    if ( trace.enabled() )  trace.log( title + ".isCaseSensitive: " + sensitive );
    return( sensitive );
} // isCaseSensitive

/*
** Name: isSigned
**
** Description:
**	Determine if column is a signed numeric.
**
** Input:
**	column	    Column index.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if column is a signed numeric, false otherwise.
**
** History:
**	17-May-99 (gordy)
**	    Created.
*/

public boolean
isSigned( int column )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".isSigned( " + column + " )" );
    column = columnMap( column );
    boolean signed = false;

    switch( desc[ column ].sql_type )
    {
    case Types.TINYINT :
    case Types.SMALLINT :
    case Types.INTEGER :
    case Types.BIGINT :
    case Types.REAL :
    case Types.FLOAT :
    case Types.DOUBLE :
    case Types.NUMERIC :
    case Types.DECIMAL :
	signed = true;	
	break;
    }

    if ( trace.enabled() )  trace.log( title + ".isSigned: " + signed );
    return( signed );
} // isSigned

/*
** Name: isCurrency
**
** Description:
**	Determine if column is a money value.
**
** Input:
**	column	    Column index.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if column a money type, false otherwise.
**
** History:
**	17-May-99 (gordy)
**	    Created.
*/

public boolean
isCurrency( int column )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".isCurrency( " + column + " )" );
    column = columnMap( column );
    boolean money = (desc[ column ].dbms_type == DBMS_TYPE_MONEY);
    if ( trace.enabled() )  trace.log( title + ".isCurrency: " + money );
    return( money );
} // isCurrency

/*
** Name: isAutoIncrement
**
** Description:
**	Determine if column is automatically numbered.
**
** Input:
**	column	    Column index.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if column is automatically numbered, false otherwise.
**
** History:
**	17-May-99 (gordy)
**	    Created.
*/

public boolean
isAutoIncrement( int column )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".isAutoIncrement( " + column + " ): " + false );
    column = columnMap( column );
    return( false );
} // isAutoIncrement

/*
** Name: isReadOnly
**
** Description:
**	Determine if column is read-only.
**
** Input:
**	column	    Column index.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if column is read-only, false otherwise.
**
** History:
**	17-May-99 (gordy)
**	    Created.
*/

public boolean
isReadOnly( int column )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".isReadOnly( " + column + " ): " + false );
    column = columnMap( column );
    return( false );
} // isReadOnly

/*
** Name: isWritable
**
** Description:
**	Determine if column may be updated.
**
** Input:
**	column	    Column index.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if column may be updated, false otherwise.
**
** History:
**	17-May-99 (gordy)
**	    Created.
*/

public boolean
isWritable( int column )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".isWritable( " + column + " ): " + true );
    column = columnMap( column );
    return( true );
} // isWritable

/*
** Name: isDefinitelyWritable
**
** Description:
**	Determine if column update will always succeed.
**
** Input:
**	column	    Column index.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if column can be updated, false otherwise.
**
** History:
**	17-May-99 (gordy)
**	    Created.
*/

public boolean
isDefinitelyWritable( int column )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".isDefinitelyWritable( " + column + " ): " + true );
    column = columnMap( column );
    return( true );
} // isDefinitelyWritable


/*
** Name: resize
**
** Description:
**	Adjusts the number of column descriptors.  Where possible,
**	the original column descriptor information is retained.
**
** Input:
**	count	Number of descriptors.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	16-Apr-01 (gordy)
**	    Created.
*/

public void
resize( short count )
{
    if ( count == this.count )  return;
    Desc desc[] = new Desc[ count ];

    if ( trace.enabled( 4 ) )  
	trace.write( tr_id + ": resize " + this.count + " to " + count );

    for( short col = 0; col < count; col++ )
	if ( col < this.count )
	    desc[ col ] = this.desc[ col ];
	else
	    desc[ col ] = new Desc();

    this.count = count;
    this.desc = desc;
    return;
} // resize


/*
** Name: setColumnInfo
**
** Description:
**	Set the descriptor information for a column.
** 
** Input:
**	name	    Name of column.
**	idx	    Column index;
**	stype	    SQL Type.
**	dtype	    DBMS Type.
**	len	    Length.
**	prec	    Precision.
**	scale	    Scale.
**	nullable    Nullable?
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	17-May-99 (gordy)
**	    Created.
*/

public void 
setColumnInfo
( 
    String  name, 
    int	    idx, 
    int	    stype, 
    short   dtype, 
    short   len, 
    byte    prec,
    byte    scale, 
    boolean nullable
)
throws EdbcEx
{
    idx = columnMap( idx );
    desc[ idx ].name = name;
    desc[ idx ].sql_type = stype;
    desc[ idx ].dbms_type = dtype;
    desc[ idx ].length = len;
    desc[ idx ].precision = prec;
    desc[ idx ].scale = scale;
    desc[ idx ].nullable = nullable;
    return;
}


/*
** Name: columnMap
**
** Description:
**	Determines if a given column index is a part of the result
**	set and maps the external index to the internal index.
**
** Input:
**	index	    External column index [1,count].
**
** Output:
**	None.
**
** Returns:
**	int	    Internal column index [0,count - 1].
**
** History:
**	17-May-99 (gordy)
**	    Created.
*/

private int
columnMap( int index )
    throws EdbcEx
{
    if ( index < 1  ||  index > count )
	throw EdbcEx.get( E_JD0007_INDEX_RANGE );
    return( index - 1 );
} // columnMap


/*
** Name: load
**
** Description:
**	Reads the result-set descriptor (JDBC_MSG_DESC) from the 
**	server connection and builds an EdbcRSMD.
**	
**
** Input:
**	dbc	    Database Connection
**	trace	    Connection tracing.
**
** Output:
**	None.
**
** Returns:
**	EdbcRSMD    Result-set Meta-data.
**
** History:
**	17-May-99 (gordy)
**	    Created.
**	25-Aug-00 (gordy)
**	    Nullable byte converted to flags.
**	 4-Oct-00 (gordy)
**	    Create unique ID for standardized internal tracing.
**	28-Mar-01 (gordy)
**	    Tracing added as a parameter.
**	16-Apr-01 (gordy)
**	    Converted from constructor to factory method.
*/

public static EdbcRSMD
load( DbConn dbc, EdbcTrace trace )
    throws EdbcEx
{
    short	count = dbc.readShort();
    EdbcRSMD	rsmd = new EdbcRSMD( count, trace );

    for( short col = 1; col <= count; col++ )
    {
	short	sql_type = dbc.readShort();
	short	dbms_type = dbc.readShort();
	short	length = dbc.readShort();
	byte	precision = dbc.readByte();
	byte	scale = dbc.readByte();
	boolean	nullable = ((dbc.readByte() & JDBC_DSC_NULL) != 0) 
			       ? true : false;
	String	name = dbc.readString();

	rsmd.setColumnInfo( name, col, sql_type, dbms_type, 
			    length, precision, scale, nullable );
    }

    return( rsmd );
} // load


/*
** Name: reload
**
** Description:
**	Reads a result-set descriptor from a database connection
**	and loads an existing RSMD with the info.
**
** Input:
**	dbc	Database connection.
**
** Output:
**	rsmd	Result-set Meta-data.
**
** Returns:
**	void.
**
** History:
**	16-Apr-01 (gordy)
**	    Created.
*/

public static void
reload( DbConn dbc, EdbcRSMD rsmd )
    throws EdbcEx
{
    short	count = dbc.readShort();
    short	common = (short)Math.min( count, rsmd.count );

    if ( count != rsmd.count )  rsmd.resize( count );

    for( short col = 0; col < count; col++ )
    {
	short	sql_type = dbc.readShort();
	short	dbms_type = dbc.readShort();
	short	length = dbc.readShort();
	byte	precision = dbc.readByte();
	byte	scale = dbc.readByte();
	boolean	nullable = ((dbc.readByte() & JDBC_DSC_NULL) != 0) 
			       ? true : false;
	String	name = dbc.readString();

	if ( col < common  &&  rsmd.trace.enabled( 5 ) )
	{
	    if ( sql_type != rsmd.desc[ col ].sql_type )
		rsmd.trace.write( rsmd.tr_id + ": reload[" + col + 
				  "] sql_type " + rsmd.desc[col].sql_type + 
				  " => " + sql_type );

	    if ( dbms_type != rsmd.desc[ col ].dbms_type )
		rsmd.trace.write( rsmd.tr_id + ": reload[" + col + 
				  "] dbms_type " + rsmd.desc[col].dbms_type + 
				  " => " + dbms_type );

	    if ( length != rsmd.desc[ col ].length )
		rsmd.trace.write( rsmd.tr_id + ": reload[" + col + 
				  "] length " + rsmd.desc[col].length + 
				  " => " + length );

	    if ( precision != rsmd.desc[ col ].precision )
		rsmd.trace.write( rsmd.tr_id + ": reload[" + col + 
				  "] precision " + rsmd.desc[col].precision + 
				  " => " + precision );

	    if ( scale != rsmd.desc[ col ].scale )
		rsmd.trace.write( rsmd.tr_id + ": reload[" + col + 
				  "] scale " + rsmd.desc[col].scale + 
				  " => " + scale );

	    if ( nullable != rsmd.desc[ col ].nullable )
		rsmd.trace.write( rsmd.tr_id + ": reload[" + col + 
				  "] nullable " + rsmd.desc[col].nullable + 
				  " => " + nullable );
	}

	rsmd.setColumnInfo( name, col + 1, sql_type, dbms_type, 
			    length, precision, scale, nullable );
    }

    return;
}


/*
** Name: Desc
**
** Description:
**	Describes a single column in the result set.
**
** History:
**	17-May-99 (gordy)
**	    Created.
*/

private static class 
Desc
{
    int	    sql_type = Types.OTHER;
    short   dbms_type = 0;
    short   length = 0;
    byte    precision = 0;
    byte    scale = 0;
    boolean nullable = false;
    String  name = null;
} // class Desc

} // class EdbcRSMD
