/*
** Copyright (c) 1999, 2009 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: JdbcRSMD
**
** Description:
**	Defines class which implements the JDBC ResultSetMetaData interface.
**
**  Classes:
**
**	JdbcRSMD	ResultSet meta-data.
**	Desc		Column descriptor.
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
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	20-Mar-03 (gordy)
**	    Updated to JDBC 3.0.
**	16-Jun-06 (gordy)
**	    ANSI Date/Time data type support.
**	25-Aug-06 (gordy)
**	    Support parameter modes.
**	15-Nov-06 (gordy)
**	    Support LOB Locators.
**	 4-May-07 (gordy)
**	    Set class access for reflection.
**      05-Jan-09 (rajus01) SIR 121238
**          - Added new JDBC 4.0 methods to avoid compilation errors with
**            JDK 1.6. The new methods return E_GC4019 error for now.
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to
**            support JDBC 4.0 SQLException hierarchy.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	26-Oct-09 (gordy)
**	    Added supprort for boolean data type.
*/

import	java.math.BigDecimal;
import	java.sql.ResultSetMetaData;
import	java.sql.ParameterMetaData;
import	java.sql.Clob;
import	java.sql.Blob;
import	java.sql.Date;
import	java.sql.Time;
import	java.sql.Timestamp;
import	java.sql.Types;
import	java.sql.Wrapper;
import	java.sql.SQLException;
import	com.ingres.gcf.util.DbmsConst;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.util.GcfErr;
import	com.ingres.gcf.util.IdMap;
import	com.ingres.gcf.dam.MsgConst;
import	com.ingres.gcf.dam.MsgConn;


/*
** Name: JdbcRSMD
**
** Description:
**	JDBC driver class which implements the ResultSetMetaData interface.
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
**	getColumnDisplaySize	Returns display width of a column.
**	getColumnLabel	    Returns title for a column.
**	getColumnName	    Returns name for a column.
**	getSchemaName	    Returns schema name in which the column resides.
**	getPrecision	    Returns number of digits for a numeric column.
**	getScale	    Returns number of digits following decimal point.
**	getTableName	    Returns table name in which the column resides.
**	getCatalogName	    Returns catalog name in which the column resides.
**	getColumnType	    Returns SQL type for a column.
**	getColumnTypeName   Returns host DBMS type for a column.
**	isReadOnly	    Determine if a column is read-only.
**	isWritable	    Determine if a column may be updated.
**	isDefinitelyWritable	Determine if column update always succeeds.
**	getColumnClassName  Returns class name of object from getObject().
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
**	setColumnInfo	    Set Column Information.
**	getParameterMode    Returns the parameter mode for a column.
**
**  Private Data:
**
**	trace		    Tracing.
**	title		    Class title for tracing.
**	tr_id		    Tracing ID.
**	inst_count	    Number of instances.
**	typeMap		    Mapping table for most DBMS datatypes.
**	intMap		    Mapping table for DBMS integer types.
**	floatMap	    Mapping table for DBMS float types.
**
**  Private Methods:
**
**	resize		    Set number of column descriptors.
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
**	31-Oct-02 (gordy)
**	    Changed package fields to public (protection controlled by
**	    package access on class).  Made reload() an instance method
**	    and made resize() private.
**	25-Aug-06 (gordy)
**	    Added getParameterMode().
**	 4-May-07 (gordy)
**	    Class is exposed outside package, permit access.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Moved map tables from public interface to private fields.
**	26-Oct-09 (gordy)
**	    Support BOOLEAN data type.
*/

public class
JdbcRSMD
    implements ResultSetMetaData, MsgConst, DbmsConst, GcfErr
{

    /*
    ** The following members are public to provide easy access 
    ** to the meta-data to other parts of the driver.
    */
    public short		count = 0;	// Number of descriptors.
    public Desc			desc[];		// Descriptors.

    private DrvTrace		trace = null;
    private String		title = null;
    private String		tr_id = null;

    private static int		inst_count = 0;

    private static IdMap	typeMap[] =
    {
	new IdMap( DBMS_TYPE_BOOL,		DBMS_TYPSTR_BOOL ),
	new IdMap( DBMS_TYPE_IDATE,		DBMS_TYPSTR_IDATE ),
	new IdMap( DBMS_TYPE_ADATE,		DBMS_TYPSTR_ADATE ),
	new IdMap( DBMS_TYPE_MONEY,		DBMS_TYPSTR_MONEY ),
	new IdMap( DBMS_TYPE_TMWO,		DBMS_TYPSTR_TMWO ),
	new IdMap( DBMS_TYPE_TMTZ,		DBMS_TYPSTR_TMTZ ),
	new IdMap( DBMS_TYPE_TIME,		DBMS_TYPSTR_TIME ),
	new IdMap( DBMS_TYPE_TSWO,		DBMS_TYPSTR_TSWO ),
	new IdMap( DBMS_TYPE_DECIMAL,		DBMS_TYPSTR_DECIMAL ),
	new IdMap( DBMS_TYPE_LOGKEY,		DBMS_TYPSTR_LOGKEY ),
	new IdMap( DBMS_TYPE_TBLKEY,		DBMS_TYPSTR_TBLKEY ),
	new IdMap( DBMS_TYPE_TSTZ,		DBMS_TYPSTR_TSTZ ),
	new IdMap( DBMS_TYPE_TS,		DBMS_TYPSTR_TS ),
	new IdMap( DBMS_TYPE_CHAR,		DBMS_TYPSTR_CHAR ),
	new IdMap( DBMS_TYPE_VARCHAR,		DBMS_TYPSTR_VARCHAR ),
	new IdMap( DBMS_TYPE_LONG_CHAR,		DBMS_TYPSTR_LONG_CHAR ),
	new IdMap( DBMS_TYPE_LCLOC,		DBMS_TYPSTR_LCLOC ),
	new IdMap( DBMS_TYPE_BYTE,		DBMS_TYPSTR_BYTE ),
	new IdMap( DBMS_TYPE_VARBYTE,		DBMS_TYPSTR_VARBYTE ),
	new IdMap( DBMS_TYPE_LONG_BYTE,		DBMS_TYPSTR_LONG_BYTE ),
	new IdMap( DBMS_TYPE_LBLOC,		DBMS_TYPSTR_LBLOC ),
	new IdMap( DBMS_TYPE_NCHAR,		DBMS_TYPSTR_NCHAR ),
	new IdMap( DBMS_TYPE_NVARCHAR,		DBMS_TYPSTR_NVARCHAR ),
	new IdMap( DBMS_TYPE_LONG_NCHAR,	DBMS_TYPSTR_LONG_NCHAR ),
	new IdMap( DBMS_TYPE_LNLOC,		DBMS_TYPSTR_LNLOC ),
	new IdMap( DBMS_TYPE_C,			DBMS_TYPSTR_C ),
	new IdMap( DBMS_TYPE_INTYM,		DBMS_TYPSTR_INTYM ),
	new IdMap( DBMS_TYPE_INTDS,		DBMS_TYPSTR_INTDS ),
	new IdMap( DBMS_TYPE_TEXT,		DBMS_TYPSTR_TEXT ),
	new IdMap( DBMS_TYPE_LONG_TEXT,		DBMS_TYPSTR_LONG_TEXT ),
    };

    private static IdMap	intMap[] =
    {
	new IdMap( 1,	DBMS_TYPSTR_INT1 ),
	new IdMap( 2,	DBMS_TYPSTR_INT2 ),
	new IdMap( 4,	DBMS_TYPSTR_INT4 ),
	new IdMap( 8,	DBMS_TYPSTR_INT8 ),
    };

    private static IdMap	floatMap[] =
    {
	new IdMap( 4,	DBMS_TYPSTR_FLT4 ),
	new IdMap( 8,	DBMS_TYPSTR_FLT8 ),
    };


/*
** Name: load
**
** Description:
**	Reads the result-set descriptor (MSG_DESC) from the server
**	connection and builds an ResultSet meta-data object.
**	
**
** Input:
**	conn	    Database Connection
**
** Output:
**	None.
**
** Returns:
**	JdbcRSMD    Result-set Meta-data.
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
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	16-Jun-06 (gordy)
**	    ANSI Date/Time data type support.  Map intervals to varchar.
**	25-Aug-06 (gordy)
**	    Save flags to make parameter modes available.
**	15-Nov-06 (gordy)
**	    Support LOB Locators.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, restrict constructor access.
*/

static JdbcRSMD	// package access
load( DrvConn conn )
    throws SQLException
{
    MsgConn	msg = conn.msg;
    short	count = msg.readShort();
    JdbcRSMD	rsmd = new JdbcRSMD( count, conn.trace );

    for( short col = 0; col < count; col++ )
    {
	short	sql_type = msg.readShort();
	short	dbms_type = msg.readShort();
	short	length = msg.readShort();
	byte	precision = msg.readByte();
	byte	scale = msg.readByte();
	byte	flags = msg.readByte();
	String	name = msg.readString();

	switch( sql_type )
	{
	case Types.LONGVARCHAR :
	    switch( dbms_type )
	    {
	    case DBMS_TYPE_LCLOC :
	    case DBMS_TYPE_LNLOC :  sql_type = Types.CLOB;	break;
	    }
	    break;

	case Types.LONGVARBINARY :
	    switch( dbms_type )
	    {
	    case DBMS_TYPE_LBLOC :  sql_type = Types.BLOB;	break;
	    }
	    break;

	case Types.TIMESTAMP :
	    if ( dbms_type == 0 )  dbms_type = DBMS_TYPE_IDATE;
	    break;

	case SQL_INTERVAL :
	    /*
	    ** Intervals are not supported directly by JDBC
	    */
	    sql_type = Types.VARCHAR;
	    length = (short)((dbms_type == DBMS_TYPE_INTYM) ? 8 : 24);
	    precision = scale = 0;
	    break;
	}

	rsmd.desc[ col ].name = name;
	rsmd.desc[ col ].sql_type = sql_type;
	rsmd.desc[ col ].dbms_type = dbms_type;
	rsmd.desc[ col ].length = length;
	rsmd.desc[ col ].precision = precision;
	rsmd.desc[ col ].scale = scale;
	rsmd.desc[ col ].flags = flags;
    }

    return( rsmd );
} // load


/*
** Name: JdbcRSMD
**
** Description:
**	Class constructor.  Allocates descriptors as requested
**	by the caller.  It is the callers responsibility to
**	populate the descriptor information.
**	
**
** Input:
**	count	Number of descriptors.
**	trace	Tracing output.
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
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, restrict constructor access.
*/

// package access
JdbcRSMD( short count, DrvTrace trace )
{
    this.count = count;
    this.trace = trace;
    
    desc = new Desc[ count ];
    for( int col = 0; col < count; col++ )  desc[ col ] = new Desc();

    title = trace.getTraceName() + "-ResultSetMetaData[" + inst_count + "]";
    tr_id = "RSMD[" + inst_count + "]";
    inst_count++;
    return;
} // JdbcRSMD


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
** Name: reload
**
** Description:
**	Reads a result-set descriptor from a database connection
**	and loads an existing RSMD with the info.
**
** Input:
**	conn	Database connection.
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
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.  Removed RSMD parameter and
**	    changed to reload current object (instance method rather than 
**	    static).
**	16-Jun-06 (gordy)
**	    ANSI Date/Time data type support.  Map intervals to varchar.
**	25-Aug-06 (gordy)
**	    Save flags to make parameter modes available.
**	15-Nov-06 (gordy)
**	    Support LOB Locators.
*/

public void
reload( DrvConn conn )
    throws SQLException
{
    MsgConn	msg = conn.msg;
    short	new_count = msg.readShort();
    short	common = (short)Math.min( new_count, count );

    if ( new_count != count )  resize( new_count );

    for( short col = 0; col < count; col++ )
    {
	short	sql_type = msg.readShort();
	short	dbms_type = msg.readShort();
	short	length = msg.readShort();
	byte	precision = msg.readByte();
	byte	scale = msg.readByte();
	byte	flags = msg.readByte();
	String	name = msg.readString();

	switch( sql_type )
	{
	case Types.LONGVARCHAR :
	    switch( dbms_type )
	    {
	    case DBMS_TYPE_LCLOC :
	    case DBMS_TYPE_LNLOC :  sql_type = Types.CLOB;	break;
	    }
	    break;

	case Types.LONGVARBINARY :
	    switch( dbms_type )
	    {
	    case DBMS_TYPE_LBLOC :  sql_type = Types.BLOB;	break;
	    }
	    break;

	case Types.TIMESTAMP :
	    if ( dbms_type == 0 )  dbms_type = DBMS_TYPE_IDATE;
	    break;

	case SQL_INTERVAL :
	    /*
	    ** Intervals are not supported directly by JDBC
	    */
	    sql_type = Types.VARCHAR;
	    length = (short)((dbms_type == DBMS_TYPE_INTYM) ? 8 : 15);
	    precision = scale = 0;
	    break;
	}

	if ( col < common  &&  trace.enabled( 5 ) )
	{
	    if ( sql_type != desc[ col ].sql_type )
		trace.write( tr_id + ": reload[" + col + "] sql_type " + 
			     desc[col].sql_type + " => " + sql_type );

	    if ( dbms_type != desc[ col ].dbms_type )
		trace.write( tr_id + ": reload[" + col + "] dbms_type " + 
			     desc[col].dbms_type + " => " + dbms_type );

	    if ( length != desc[ col ].length )
		trace.write( tr_id + ": reload[" + col + "] length " + 
			     desc[col].length + " => " + length );

	    if ( precision != desc[ col ].precision )
		trace.write( tr_id + ": reload[" + col + "] precision " + 
			     desc[col].precision + " => " + precision );

	    if ( scale != desc[ col ].scale )
		trace.write( tr_id + ": reload[" + col + "] scale " + 
			     desc[col].scale + " => " + scale );

	    if ( flags != desc[ col ].flags )
		trace.write( tr_id + ": reload[" + col + "] flags " + 
			     desc[col].flags + " => " + flags );
	}

	desc[ col ].name = name;
	desc[ col ].sql_type = sql_type;
	desc[ col ].dbms_type = dbms_type;
	desc[ col ].length = length;
	desc[ col ].precision = precision;
	desc[ col ].scale = scale;
	desc[ col ].flags = flags;
    }

    return;
}


/*
** Name: setColumnInfo
**
** Description:
**	Set the descriptor information for a column.
** 
** Input:
**	name		Name of column.
**	idx		Column index;
**	sql_type	SQL Type.
**	dbms_type	DBMS Type.
**	length		Length.
**	precision	Precision.
**	scale		Scale.
**	nullable	Nullable?
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
**	25-Aug-06 (gordy)
**	    Set NULL flag when nullable.
*/

public void 
setColumnInfo
( 
    String  name, 
    int	    idx, 
    int	    sql_type, 
    short   dbms_type, 
    short   length, 
    byte    precision,
    byte    scale, 
    boolean nullable
)
    throws SQLException
{
    idx = columnMap( idx );
    desc[ idx ].name = name;
    desc[ idx ].sql_type = sql_type;
    desc[ idx ].dbms_type = dbms_type;
    desc[ idx ].length = length;
    desc[ idx ].precision = precision;
    desc[ idx ].scale = scale;
    desc[ idx ].flags = (byte)(nullable ? MSG_DSC_NULL : 0);
    return;
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
	trace.log( title + ".getTableName( " + column + " )" );
    column = columnMap( column );
    if ( trace.enabled() )  trace.log( title + ".getTableName: ''" );
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
	trace.log( title + ".getSchemaName( " + column + " )" );
    column = columnMap( column );
    if ( trace.enabled() )  trace.log( title + ".getSchemaName: ''" );
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
	trace.log( title + ".getCatalogName( " + column + " )" );
    column = columnMap( column );
    if ( trace.enabled() )  trace.log( title + ".getCatalogName: ''" );
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
**	20-Mar-03 (gordy)
**	    JDBC added BOOLEAN data type, treat same as BIT.
**	15-Nov-06 (gordy)
**	    Support LOB Locators via Clob/Blob.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Removed unneeded initialization.
*/

public String
getColumnClassName( int column )
    throws SQLException
{
    String  name;
    Class   oc;

    if ( trace.enabled() )  
	trace.log( title + ".getColumnClassName( " + column + " )" );
    
    switch( desc[ (column = columnMap( column )) ].sql_type )
    {
    case Types.BOOLEAN :
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
    case Types.LONGVARBINARY :	oc = byte[].class;	break;

    case Types.DATE :		oc = Date.class;	break;
    case Types.TIME :		oc = Time.class;	break;
    case Types.TIMESTAMP :	oc = Timestamp.class;	break;

    case Types.CLOB :		oc = Clob.class;	break;
    case Types.BLOB :		oc = Blob.class;	break;

    default :		    
	if ( trace.enabled( 1 ) )
	    trace.write(title + ": invalid SQL type " + desc[column].sql_type);
	throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
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
**      04-Jun-08 (rajus01) SD issue 128696, Bug 120463
**          Added precision values of DATE/TIME/TIMESTAMP datatypes.
*/

public int
getPrecision( int column )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getPrecision( " + column + " )" );
    column = columnMap( column );
    int precision = 0;

    switch( desc[ column ].sql_type )
    {
    case Types.DATE :		precision = 10;	break;
    case Types.TIME :		precision = 8;	break;
    case Types.TIMESTAMP :	precision = 29;	break;
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
	switch( desc[ column ].dbms_type )
	{
	case DBMS_TYPE_NCHAR :
	    precision = desc[ column ].length / 2;
	    break;

	default :
	    precision = desc[ column ].length;
	    break;
	}
	break;

    case Types.VARCHAR :
	switch( desc[ column ].dbms_type )
	{
	case DBMS_TYPE_NVARCHAR :
	    precision = desc[ column ].length / 2;
	    break;

	default :
	    precision = desc[ column ].length;
	    break;
	}
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
    if ( trace.enabled() )  
	trace.log( title + ".getScale: " + desc[ column ].scale );
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
**	20-Mar-03 (gordy)
**	    JDBC added BOOLEAN data type, treat same as BIT.
**	15-Nov-06 (gordy)
**	    Support LOB locators via Blob/Clob.
**      04-Jun-08 (rajus01) SD issue 128696, Bug 120463
**          Fixed the value for TIMESTAMP datatype.
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
    case Types.BOOLEAN :
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
    case Types.TIMESTAMP :	len = 29;	break;
    case Types.BINARY :
    case Types.VARBINARY :	len = desc[ column ].length;	break;

    case Types.CLOB :
    case Types.BLOB :
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

    if ( trace.enabled() )  trace.log(title + ".getColumnDisplaySize: " + len);
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
**	25-Aug-06 (gordy)
**	    Check for NULL flag.
*/

public int
isNullable( int column )
    throws SQLException
{
    if ( trace.enabled() )  trace.log(title + ".isNullable( " + column + " )");

    column = columnMap( column );
    int nulls = ((desc[ column ].flags & MSG_DSC_NULL) != 0) ? columnNullable 
							     : columnNoNulls;

    if ( trace.enabled() )  trace.log( title + ".isNullable: " + nulls );
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
	trace.log( title + ".isSearchable( " + column + " )" );
    column = columnMap( column );
    if ( trace.enabled() )  trace.log( title + ".isSearchable: " + true );
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
**	15-Nov-06 (gordy)
**	    Support LOB Locators via Blob/Clob.
*/

public boolean
isCaseSensitive( int column )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".isCaseSensitive( " + column + " )" );
    column = columnMap( column );
    boolean sensitive = false;

    switch( desc[ column ].sql_type )
    {
    case Types.CHAR :
    case Types.VARCHAR :
    case Types.LONGVARCHAR :
    case Types.CLOB :
	sensitive = true;
	break;
    }

    if ( trace.enabled() )  trace.log(title + ".isCaseSensitive: " + sensitive);
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
    if ( trace.enabled() )  trace.log(title + ".isCurrency( " + column + " )");
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
	trace.log( title + ".isAutoIncrement( " + column + " )" );
    column = columnMap( column );
    if ( trace.enabled() )  trace.log( title + ".isAutoIncrement: " + false );
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
    if ( trace.enabled() )  trace.log(title + ".isReadOnly( " + column + " )");
    column = columnMap( column );
    if ( trace.enabled() )  trace.log( title + ".isReadOnly: " + false );
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
    if ( trace.enabled() )  trace.log(title + ".isWritable( " + column + " )");
    column = columnMap( column );
    if ( trace.enabled() )  trace.log( title + ".isWritable: " + true );
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
	trace.log( title + ".isDefinitelyWritable( " + column + " )" );
    column = columnMap( column );
    if ( trace.enabled() )  trace.log(title + ".isDefinitelyWritable: " + true);
    return( true );
} // isDefinitelyWritable


/*
** Name: getParameterMode
**
** Description:
**	Returns the parameter mode of a column.
**	Returns one of the following values:
**
**	    parameterModeIn
**	    parameterModeOut
**	    parameterModeInOut
**	    parameterModeUnknown
**
** Input:
**	column	Column index
**
** Output:
**	None.
**
** Returns:
**	int	Parameter mode.
**
** History:
**	25-Aug-06 (gordy)
**	    Created.
*/

public int
getParameterMode( int column )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getParameterMode( " + column + " )" );

    column = columnMap( column );
    int mode = ParameterMetaData.parameterModeUnknown;

    if ( (desc[ column ].flags & MSG_DSC_PIO) != 0 )
	mode = ParameterMetaData.parameterModeInOut;
    else  if ( (desc[ column ].flags & MSG_DSC_PIN) != 0 )
    {
	/*
	** According to the DAM protocol, IN/OUT parameters
	** are indicated by the PIO flag.  Just to be safe,
	** IN/OUT mode is also set if both the PIN and POUT 
	** flags are set.
	*/
    	if ( (desc[ column ].flags & MSG_DSC_POUT) != 0 )
	    mode = ParameterMetaData.parameterModeInOut;
	else
	    mode = ParameterMetaData.parameterModeIn;
    }
    else  if ( (desc[ column ].flags & MSG_DSC_POUT) != 0 )
	mode = ParameterMetaData.parameterModeOut;

    if ( trace.enabled() )  
	trace.log( title + ".getParameterMode: " + mode );
    return( mode );
} // getParameterMode


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
**	31-Oct-02 (gordy)
**	    Made private.
*/

private void
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
    throws SQLException
{
    if ( index < 1  ||  index > count )
	throw SqlExFactory.get( ERR_GC4011_INDEX_RANGE );
    return( index - 1 );
} // columnMap


/*
** Name: isWrapperFor
**
** Description:
**      Returns true if this either implements the interface argument or is
**      directly or indirectly a wrapper for an object that does. Returns
**      false otherwise.
**
** Input:
**      iface   Class defining an interface
**
** Output:
**      None.
**
** Returns:
**      true or false.
**
** History:
**      23-Jun-09 (rajus01)
**         Implemented.
*/
public boolean
isWrapperFor(Class<?> iface)
throws SQLException
{
    /*
    ** This approach seems to work for classes that actually don't
    ** wrap anything.
    */
    if( iface != null )
	return( iface.isInstance( this ) );

    throw SqlExFactory.get(ERR_GC4010_PARAM_VALUE);

} //isWrapperFor

/*
** Name: unwrap
**
** Description:
**      Returns an object that implements the given interface to allow
**      access to non-standard methods, or standard methods not exposed by the
**      proxy. If the receiver implements the interface then the result is the
**      receiver or a proxy for the receiver. If the receiver is a wrapper and
**      the wrapped object implements the interface then the result is the
**      wrapped object or a proxy for the wrapped object. Otherwise return the
**      the result of calling unwrap recursively on the wrapped object or
**      a proxy for that result. If the receiver is not a wrapper and does not
**      implement the interface, then an SQLException is thrown.
**
** Input:
**      iface   A Class defining an interface that the result must implement.
**
** Output:
**      None.
**
** Returns:
**      An object that implements the interface.
**
** History:
**      23-Jun-09 (rajus01)
**         Implemented.
*/
public <T> T
unwrap(Class<T> iface)
throws SQLException
{
    /*
    ** This approach seems to work for classes that actually don't
    ** wrap anything.
    */
    if( iface != null )
    {
	if( ! iface.isInstance( this ) )
	    throw SqlExFactory.get(ERR_GC4023_NO_OBJECT); 
	return( iface.cast(this) );
    }

    throw SqlExFactory.get(ERR_GC4010_PARAM_VALUE);

} //unwrap

/*
** Name: Desc
**
** Description:
**	Represents a column in a result-set.
**
** History:
**	17-May-99 (gordy)
**	    Created.
**	25-Aug-06 (gordy)
**	    Changed nullable to flags to hold parameter modes.
**	27-Apr-07 (rajus01)
**	    Changed class modifier to package access to avoid compilation errors
**	    using jdk 1.4_06 and higher..
*/

static class // package access
Desc
{

    int	    sql_type = Types.OTHER;
    short   dbms_type = 0;
    short   length = 0;
    byte    precision = 0;
    byte    scale = 0;
    byte    flags = 0;
    String  name = null;

} // class Desc

} // class JdbcRSMD
