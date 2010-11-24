/*
** Copyright (c) 1999, 2010 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: ParamSet
**
** Description:
**	Defines classes representing a set of query parameters.
**
**  Classes:
**
**	ParamSet    A parameter data-set.
**	Param	    An entry in a parameter data-set.
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
**	 8-Jan-01 (gordy)
**	    Extracted additional parameter handling from EdbcPrep.
**	11-Apr-01 (gordy)
**	    Do not include time with DATE datatypes.
**	10-May-01 (gordy)
**	    Added support for UCS2 datatypes.
**	 1-Jun-01 (gordy)
**	    JDBC requires long strings to be converted to BLOBs, but
**	    not the other way around.  Removed conversion of short
**	    BLOBs to fixed length strings, which removed the need to
**	    track stream lengths.  No longer need the wrapper classes
**	    and interfaces associated with stream lengths, greatly
**	    simplifying this implementation.
**	13-Jun-01 (gordy)
**	    No longer need setUnicode() and UniRdr class.  The JDBC 2.1
**	    spec requires UTF-8 encoding for unicode streams, which can
**	    be read using the standard class java.io.InputStreamReader.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	19-Feb-03 (gordy)
**	    Updated to JDBC 3.0.
**	15-Apr-03 (gordy)
**	    Date formatters require synchronization.
**	26-Jun-03 (gordy)
**	    All CHAR/VARCHAR coercions need alt storage handling.
**	11-Jul-03 (wansh01)
**	    all BINARY/CHAR coercions using getBytes() instead of str2bin().
**	21-Jul-03 (gordy)
**	    Permit conversion from empty string to Date/Time/Timestamp.
**	12-Sep-03 (gordy)
**	    Date formatter replaced with SqlDates utility.
**	 1-Dec-03 (gordy)
**	    Reworked to incorporate support for SqlData objects.  Moved
**	    some functionality into SqlData objects and incorporated
**	    functionality from statement parameter handling.
**	23-Jan-04 (gordy)
**	    Fixed problem where NULL CHAR/BINARY values were being sent
**	    as zero length values.  Restored date/time coercions which
**	    were not being done by the SQL storage class.
**	15-Mar-04 (gordy)
**	    Added support for BIGINT values sent to DBMS.
**	28-Apr-04 (wansh01)
**	    Added type as an input for setObejct().
**	    The input type is used for init and coerce(). 	
**	29-Apr-04 (wansh01)
**	    Changed prec calculation for BigDecimal. 
**	20-May-04 (gordy)
**	    Allow for coerce of empty strings to support Ingres empty dates.
**	28-May-04 (gordy)
**	    Add max column lengths for NCS columns.
**	11-Aug-04 (gordy)
**	    The described length of NCS columns need to be in bytes.
**	19-Jun-06 (gordy)
**	    ANSI Date/Time data type support.
**	28-Jun-06 (gordy)
**	    OUT procedure parameter support.
**	22-Sep-06 (gordy)
**	    Fixed scale of TIMESTAMP values.
**	15-Nov-06 (gordy)
**	    Support LOB Locators via BLOB/CLOB.
**	24-Dec-08 (gordy)
**	    Use date/time formatter associated with connection.
**      05-Jan-09 (rajus01) SIR 121238
**          - Added new JDBC 4.0 methods to avoid compilation errors with
**            JDK 1.6. The new methods return E_GC4019 error for now.
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to
**            support JDBC 4.0 SQLException hierarchy.
**	12-Mar-09 (gordy)
**	    Restrict use of BLOB/CLOB types internally to driver generated
**	    locators.  Convert NULL LOB values to more compatible types.
**      10-Feb-09 (rajus01) SIR 121238
**	    New data types: NCHAR is aliased to CHAR. NVARCHAR is aliased 
**	    to VARCHAR. LONGNVARCHAR is aliased to LONGVARCHAR. NCLOB is
**	    aliased to CLOB.
**	26-Oct-09 (gordy)
**	    Fully support BOOLEAN data type.
**      11-Feb-10 (rajus01) Bug 123277
**	    Force date/time/timestamp values as INGRESDATE datatype if
**	    'send_ingres_dates' connection property is set.
**	25-Mar-10 (gordy)
**	    Added support for batch processing.
**	30-Jul-10 (gordy)
**	    Force boolean values as TINYINT datatype if 'send_integer_booleans'
**	    connection property is set.
*/


import	java.math.BigDecimal;
import	java.io.InputStream;
import	java.io.Reader;
import	java.sql.Types;
import	java.sql.Blob;
import	java.sql.Clob;
import	java.sql.Date;
import	java.sql.Time;
import	java.sql.Timestamp;
import	java.sql.SQLException;
import	java.util.TimeZone;
import	com.ingres.gcf.util.DbmsConst;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.util.GcfErr;
import	com.ingres.gcf.util.CharSet;
import	com.ingres.gcf.util.SqlData;
import	com.ingres.gcf.util.SqlNull;
import	com.ingres.gcf.util.SqlBool;
import	com.ingres.gcf.util.SqlTinyInt;
import	com.ingres.gcf.util.SqlSmallInt;
import	com.ingres.gcf.util.SqlInt;
import	com.ingres.gcf.util.SqlBigInt;
import	com.ingres.gcf.util.SqlReal;
import	com.ingres.gcf.util.SqlDouble;
import	com.ingres.gcf.util.SqlDecimal;
import	com.ingres.gcf.util.SqlByte;
import	com.ingres.gcf.util.SqlVarByte;
import	com.ingres.gcf.util.SqlLongByte;
import	com.ingres.gcf.util.SqlChar;
import	com.ingres.gcf.util.SqlNChar;
import	com.ingres.gcf.util.SqlVarChar;
import	com.ingres.gcf.util.SqlNVarChar;
import	com.ingres.gcf.util.SqlLongChar;
import	com.ingres.gcf.util.SqlLongNChar;
import	com.ingres.gcf.util.SqlDate;
import	com.ingres.gcf.util.SqlTime;
import	com.ingres.gcf.util.SqlTimestamp;
import	com.ingres.gcf.util.IngresDate;
import	com.ingres.gcf.util.SqlLoc;


/*
** Name: ParamSet
**
** Description:
**	Class which manages a parameter data set.
**
**	Parameter values are stored as specific class instances
**	depending on the associated SQL type.  All storage classes
**	are a sub-class of com.ingres.gcf.util.SqlData.  Some types 
**	have an alternate storage class in addition to the primary 
**	storage class.  An alternate storage class is used when 
**	explicitly requested, or by default when appropriate for the 
**	associated connection.  The supported SQL types along with 
**	their primary and alternate storage classes are as follows:
**	
**	SQL Type	Storage Class	Alternate Class
**	-------------	-------------	---------------
**	NULL		SqlNull
**	BOOLEAN		SqlBool		SqlTinyInt
**	TINYINT		SqlTinyInt
**	SMALLINT	SqlSmallInt
**	INTEGER		SqlInt
**	BIGINT		SqlBigInt	SqlDouble
**	REAL		SqlReal
**	DOUBLE		SqlDouble
**	DECIMAL		SqlDecimal	SqlDouble
**	BINARY		SqlByte
**	VARBINARY	SqlVarByte
**	LONGVARBINARY	SqlLongByte
**	CHAR		SqlNChar	SqlChar
**	VARCHAR		SqlNVarChar	SqlVarChar
**	LONGVARCHAR	SqlLongNChar	SqlLongChar
**	DATE		SqlDate		IngresDate
**	TIME		SqlTime		IngresDate
**	TIMESTAMP	SqlTimestamp	IngresDate
**	BLOB		SqlBLoc
**	CLOB		SqlNLoc		SqlCLoc
**
**	The preferred alternate storage format for BIGINT is DECIMAL, 
**	but DECIMAL has it's own alternate storage format.  To make 
**	this managable, BIGINT has the same alternate format as 
**	DECIMAL, uses the same logic to select the aternate format, 
**	and is forced to the DECIMAL type when BIGINT is not 
**	supported.  This allows BIGINT to be sent to the DBMS as 
**	BIGINT, DECIMAL or DOUBLE as needed.
**
**	The following SQL types are supported on input as aliases
**	for the indicated types:
**
**	Alias Type	SQL Type
**	-------------	-------------
**	BIT		BOOLEAN
**	FLOAT		DOUBLE
**	NUMERIC		DECIMAL
**	NCHAR		CHAR
**	NVARCHAR	VARCHAR
**	LONGNVARCHAR	LONGVARCHAR
**	NCLOB		CLOB
**
**	Storage classes may also be determined by the Java object 
**	type of a parameter value when no other SQL type is known 
**	or required.  The supported Java object classes and their 
**	associated SQL types are determined by the getSqlType()
**	method of the com.ingres.gcf.util.SqlData class.
**
**  Constants:
**
**	PS_FLG_ALT		Alternate Format
**	PS_FLG_PROC_IN		Procedure IN parameter.
**	PS_FLG_PROC_OUT		Procedure OUT parameter.
**	PS_FLG_PROC_IO		Procedure INOUT parameter.
**	PS_FLG_PROC_GTT		Global Temp Table parameter.
**
**  Public Methods:
**
**	setDefaultFlags		Set default parameter flags.
**	clear			Clear all parameter settings.
**	sendDesc		Send DESC message.
**	sendData		Send DATA message.
**	getCount		Returns number of parameters.
**	isSet			Has a parameter been set.
**	getType			Returns parameter type.
**	getValue		Returns parameter value.
**	getFlags		Returns parameter flags.
**	getName			Returns parameter name.
**	init			Initialize parameter.
**	setFlags		Set parameter flags.
**	setName			Set parameter name.
**	setNull			Set parameter to NULL.
**	setBoolean		Set a boolean parameter value.
**	setByte			Set a byte parameter value.
**	setShort		Set a short parameter value.
**	setInt			Set an int parameter value.
**	setLong			Set a long parameter value.
**	setFloat		Set a float parameter value.
**	setDouble		Set a double parameter value.
**	setBigDecimal		Set a BigDecimal parameter value.
**	setString		Set a String parameter value.
**	setBytes		Set a byte array parameter value.
**	setDate			Set a Date parameter value.
**	setTime			Set a Time parameter value.
**	setTimestamp		Set a Timestamp parameter value.
**	setBinaryStream		Set a binary stream parameter value.
**	setAsciiStream		Set an ASCII stream parameter value.
**	setUnicodeStream	Set a Unicode stream parameter value.
**	setCharacterStream	Set a character stream parameter value.
**	setBlob			Set a Blob parameter value.
**	setClob			Set a Clob parameter value.
**	setObject		Set a generic Java object parameter value.
**
**  Private Data:
**
**	PS_FLG_SET		Parameter is set.
**	EXPAND_SIZE		Size to grow the parameter data array.
**
**	params			Parameter array.
**	param_cnt		Number of actual parameters.
**	dflt_flags		Default parameter flags.
**
**	emptyVarChar		Empty data values.
**	emptyNVarChar
**	emptyVarByte
**	tempLongChar		Data values to assist data conversion.
**	tempLongNChar
**	tempLongByte
**
**  Private Methods:
**
**	set			Set parameter type and storage.
**	check			Validate parameter index.
**	useAltStorage		Should alternate storage be used?
**	getStorage		Allocate storage for SQL type.
**	checkSqlType		Validate/convert parameter type.
**	coerce			Coerce parameter value to SQL type.
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
**	 8-Jan-01 (gordy)
**	    Extracted additional parameter handling from EdbcPrep.
**	    Added default parameter flags, param_flags, and max
**	    varchar string length, max_vchr_len.
**	10-May-01 (gordy)
**	    Added constants for parameter flags.  Removed setNull()
**	    method as can be done through setObject().  Added str2bin
**	    for String to BINARY conversion.
**	 1-Jun-01 (gordy)
**	    Removed setBinary(), setAscii() and setCharacter() since
**	    wrapper classes are no longer needed to track stream lengths.
**	13-Jun-01 (gordy)
**	    No longer need setUnicode() and UniRdr class.  The JDBC 2.1
**	    spec requires UTF-8 encoding for unicode streams, which can
**	    be read using the standard class java.io.InputStreamReader.
**      13-Aug-01 (loera01) SIR 105521
**          Added PS_FLG_PROC_GTT constant to support global temp table
**          parameters.
**	31-Oct-02 (gordy)
**	    Removed unused max_vchr_len and setMaxLength().
**	25-Feb-03 (gordy)
**	    Added internal PS_FLG_SET to indicate set parameters rather
**	    than relying on simply object existence which may be a hold
**	    over from a prior parameter set (cleared but not purged).
**	    Changed flags to short for additional bits.  Added char_set 
**	    to support string conversions.
**	21-Jul-03 (gordy)
**	    Added empty string constant for Ingres empty dates.
**	12-Sep-03 (gordy)
**	    Date formatter replaced with GMT indicator.
**	 1-Dec-03 (gordy)
**	    Completely reworked interface taking in aspects of statement
**	    parameter handling while moving internal data handling to 
**	    SqlData objects.  DrvObj is now base class giving access 
**	    to connection information (dropped char_set and use_gmt).  
**	    Renamed param_flags to dflt_flags.  Added tempLongChar, 
**	    tempLongNChar, tempLongByte, emptyVarChar, emptyVarByte, and 
**	    associated static initializer.  Moved getSqlType(), str2bin()
**	    and functionality of coerce() to SqlData and SqlXXX classes.  
**	    Dropped clone() method and added getStorage().  Removed old 
**	    interface of set() and setObject() and replaced with init() 
**	    and setXXX() methods (set() is now an internal method and 
**	    setObject() reflects JDBC semantics).  Incorporated sendDesc(), 
**	    sendData() and useAltStorage() from statement parameter handling.
**	23-Jan-04 (gordy)
**	    Added missing emptyNVarChar() to support 0 length NCHAR columns.
**	    Restored coerce() function to perform conversions of date/time
**	    values not supported by the common storage class IngresDate.
**	15-Mar-04 (gordy)
**	    Made checkSqlType non-static as it needs to check protocol level
**	    to determine BIGINT support (force to DECIMAL).
**	28-Jun-06 (gordy)
**	    Added PS_FLG_PROC_OUT for procedure output parameters.
**	25-Mar-10 (gordy)
**	    Added sendData() method which takes explicit header flags.
*/

class
ParamSet
    extends DrvObj
    implements DbmsConst, GcfErr
{

    /*
    ** Constants for parameter flags.
    */
    public static final short	PS_FLG_PROC_IN      = 0x01;
    public static final short	PS_FLG_PROC_OUT     = 0x02;
    public static final short	PS_FLG_PROC_IO      = 0x03;
    public static final short	PS_FLG_PROC_GTT     = 0x04;
    public static final short	PS_FLG_ALT	    = 0x10;	// Can't be set

    private static final short	PS_FLG_SET	    = 0x80;	// Internal use

    /*
    ** A CharSet is required to generate a SqlVarChar instance,
    ** but at static initialization the connection CharSet is not
    ** known.  Since this will be used to represent a zero length
    ** string, the CharSet does not actually matter so the default
    ** CharSet is used.
    */
    private static final SqlVarChar	emptyVarChar = 
					new SqlVarChar( new CharSet(), 0 );
    private static final SqlNVarChar	emptyNVarChar = new SqlNVarChar( 0 );
    private static final SqlVarByte	emptyVarByte = new SqlVarByte( 0 );

    /*
    ** While a generic Vector class is a possible implementation 
    ** for this class, a parameter data set is best represented 
    ** by a sparse array which is not well suited to the Vector
    ** class.
    */
    private static final int	EXPAND_SIZE = 10;
    private int			param_cnt = 0;
    private Param		params[];

    private short		dflt_flags = 0;		// Default flags.
    private SqlLongChar		tempLongChar = null;	// Data Conversion
    private SqlLongNChar	tempLongNChar = null;
    private SqlLongByte		tempLongByte = null;

    
/*
** Name: static
**
** Description:
**	Static initialization.  New SqlData values are initially
**	NULL and need to be cleared to represent zero length values.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
**	23-Jan-04 (gordy)
**	    Added emptyNVarChar().
*/

static
{
    emptyVarChar.clear();
    emptyNVarChar.clear();
    emptyVarByte.clear();
} // static


/*
** Name: ParamSet
**
** Description:
**	Class constructor.  Creates a parameter set of the
**	default size.  
**
** Input:
**	conn	Associated connection.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Use negative value for default size.
**	25-Feb-03 (gordy)
**	    Added char_set parameter.
**	12-Sep-03 (gordy)
**	    Date formatter replaced with GMT indicator.
**	 1-Dec-03 (gordy)
**	    Replaced connection dependent parameters with conn parameter.
*/

public
ParamSet( DrvConn conn )
{
    this( conn, EXPAND_SIZE );
} // ParamSet


/*
** Name: ParamSet
**
** Description:
**	Class constructor.  Creates a parameter set of the
**	requested size.
**
** Input:
**	conn	Associated connection.
**	size	Initial size.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Allow zero as valid initial size.
**	25-Feb-03 (gordy)
**	    Added char_set parameter.
**	12-Sep-03 (gordy)
**	    Date formatter replaced with GMT indicator.
**	 1-Dec-03 (gordy)
**	    Replaced connection dependent parameters with conn parameter.
*/

public
ParamSet( DrvConn conn, int size )
{
    super( conn );
    params = new Param[ (size < 0) ? 0 : size ];
    title = trace.getTraceName() + "-ParamSet[" + inst_id + "]";
    tr_id = "Param[" + inst_id + "]";
} // ParamSet


/*
** Name: setDefaultFlags
**
** Description:
**	Set the default parameter flags which are assigned
**	when a parameter value is assigned using a setXXX() 
**	method.
**
** Input:
**	flags	    Parameter flags.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	15-Jun-00 (gordy)
**	    Created.
**	 8-Jan-01 (gordy)
**	    Set global flags rather than specific parameter flags.
*/

public void
setDefaultFlags( short flags )
{
    dflt_flags = flags;
    return;
} // setDefaultFlags


/*
** Name: clear
**
** Description:
**	Clear stored parameter values and optionally free resources
**
** Input:
**	purge	Purge allocated data?
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Add synchronization.
**	25-Feb-03 (gordy)
**	    Clear params to reset flags.  Purge all parameters.
**	 1-Dec-03 (gordy)
**	    If not purging, save type, alt flag, and value for re-use.
*/

public synchronized void
clear( boolean purge )
{
    /*
    ** When purging, scan all parameters to get hold
    ** overs from previous sets.  Otherwise, just scan
    ** the current parameter set.
    */
    if ( purge )  param_cnt = params.length;

    for( int i = 0; i < param_cnt; i++ )  
	if ( params[ i ] != null )
	    if ( purge )  
	    {
		params[ i ].clear();
		params[ i ] = null;
	    }
	    else  synchronized( params[ i ] )
	    {
		/*
		** Type, alt, and value storage are saved for re-use.
		*/
		params[ i ].flags = (short)(params[ i ].flags & PS_FLG_ALT);
		params[ i ].name = null;
	    }

    param_cnt = 0;
    return;
} // clear


/*
** Name: getCount
**
** Description:
**	Returns the number parameters in the data set
**	(includes unset intermediate parameters).
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Number of parameters.
**
** History:
**	15-Nov-00 (gordy)
*/

public int
getCount()
{
    return( param_cnt );
} // getCount


/*
** Name: isSet
**
** Description:
**	Has the requested parameter been set?
**
** Input:
**	index	Parameter index (0 based).
**
** Output:
**	None.
**
** Returns:
**	boolean	TRUE if parameter set, FALSE otherwise.
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
**	25-Feb-03 (gordy)
**	    Check set flag.
*/

public synchronized boolean
isSet( int index )
{
    return( index < param_cnt  &&  params[ index ] != null  &&  
	    (params[ index ].flags & PS_FLG_SET) != 0 );
} // isSet


/*
** Name: getType
**
** Description:
**	Retrieve the type of a parameter.
**
** Input:
**	index	Parameter index (0 based).
**
** Output:
**	None.
**
** Returns:
**	int	Parameter type.
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    The check() method now returns parameter object.
*/

public int
getType( int index )
{
    return( check( index ).type );
} // getType


/*
** Name: getValue
**
** Description:
**	Retrieve the value of a parameter.
**
** Input:
**	index	Parameter index (0 based).
**
** Output:
**	None.
**
** Returns:
**	SqlData	Parameter value.
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    The check() method now returns parameter object.
*/

public SqlData
getValue( int index )
{
    return( check( index ).value );
} // getValue


/*
** Name: getFlags
**
** Description:
**	Retrieve the flags of a parameter.
**
** Input:
**	index	Parameter index (0 based).
**
** Output:
**	None.
**
** Returns:
**	short	Parameter flags.
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
**	25-Feb-03 (gordy)
**	    Changed flags to short for additional bits.
**	 1-Dec-03 (gordy)
**	    The check() method now returns parameter object.
*/

public short
getFlags( int index )
{
    return( check( index ).flags );
} // getFlags


/*
** Name: getName
**
** Description:
**	Retrieve the name of a parameter.
**
** Input:
**	index	Parameter index (0 based).
**
** Output:
**	None.
**
** Returns:
**	String	Parameter name.
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    The check() method now returns parameter object.
*/

public String
getName( int index )
{
    return( check( index ).name );
} // getName


/*
** Name: init
**
** Description:
**	Initialize a parameter to hold values for a given
**	SQL type.  Alternate storage format may be used
**	based on attributes of the associated connection.
**
**	The parameter is not considered to be set.  Storage
**	from prior usage may be re-used and the resulting
**	parameter value is indeterminate.  Parameter flags
**	and name are not changed.
**
** Input:
**	index	Parameter index (0 based).
**	type	SQL type.
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
*/

public void
init( int index, int type )
    throws SQLException
{
    init( index, type, useAltStorage( type ) );
    return;
} // init

    
/*
** Name: init
**
** Description:
**	Initialize a parameter to hold values for a SQL type
**	which is associated with a Java object.  Alternate 
**	storage format may be used based on attributes of the 
**	associated connection.
**
**	The parameter is not considered to be set.  Storage
**	from prior usage may be re-used and the resulting
**	parameter value is indeterminate.  Parameter flags
**	and name are not changed.
**
** Input:
**	index	Parameter index (0 based).
**	value	Java object.
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
**	12-Mar-09 (gordy)
**	    Restrict usage of BLOB/CLOB types to driver generated locators.
*/

public void
init( int index, Object value )
    throws SQLException
{
    int type = SqlData.getSqlType( value );
    boolean alt = useAltStorage( type );

    switch( type )
    {
    case Types.BLOB :
	/*
	** The BLOB type is only used internally 
	** for locators generated by the driver.
	*/
	if ( ! (value instanceof DrvBlob) )  
	{
	    type = Types.LONGVARBINARY;
	    alt = useAltStorage( type );
	}
	break;

    case Types.CLOB :
	/*
	** The CLOB type is only used internally
	** for locators generated by the driver.
	** The locator type also determines the
	** standard/alternate storage format.
	*/
	if ( value instanceof DrvClob )
	    alt = true;
	else  if ( value instanceof DrvNlob )
	    alt = false;
	else
	{
	    type = Types.LONGVARCHAR;
	    alt = useAltStorage( type );
	}
	break;
    }

    init( index, type, alt );
    return;
} // init

    
/*
** Name: init
**
** Description:
**	Initialize a parameter to hold values for a SQL type,
**	which is associated with a Java object, and optional
**	alternate storage format.
**
**	The parameter is not considered to be set.  Storage
**	from prior usage may be re-used and the resulting
**	parameter value is indeterminate.  Parameter flags
**	and name are not changed.
**
** Input:
**	index	Parameter index (0 based).
**	value	Java object.
**	alt	True for alternate storage format.
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
**	12-Mar-09 (gordy)
**	    Restrict usage of BLOB/CLOB types to driver generated locators.
*/

public void
init( int index, Object value, boolean alt )
    throws SQLException
{
    int type = SqlData.getSqlType( value );

    switch( type )
    {
    case Types.BLOB :
	/*
	** The BLOB type is only used internally 
	** for locators generated by the driver.
	*/
	if ( ! (value instanceof DrvBlob) )  type = Types.LONGVARBINARY;
	break;

    case Types.CLOB :
	/*
	** The CLOB type is only used internally
	** for locators generated by the driver.
	** The locator type also determines the
	** standard/alternate storage format and 
	** caller provided indicator is explicitly
	** overridden to match the value.
	*/
	if ( value instanceof DrvClob )
	    alt = true;
	else  if ( value instanceof DrvNlob )
	    alt = false;
	else
	    type = Types.LONGVARCHAR;
	break;
    }

    init( index, type, alt );
    return;
} // init

    
/*
** Name: init
**
** Description:
**	Initialize a parameter to hold values for a given
**	SQL type and optional alternate storage format.
**
**	The parameter is not considered to be set.  Storage
**	from prior usage may be re-used and the resulting
**	parameter value is indeterminate.  Parameter flags
**	and name are not changed.
**
** Input:
**	index	Parameter index (0 based).
**	type	SQL type.
**	alt	True for alternate storage format.
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
*/

public void
init( int index, int type, boolean alt )
    throws SQLException
{
    /*
    ** Validate index and SQL data type.  Allocates referenced
    ** parameter if necessary and standardizes the type.
    */
    Param param = check( index );
    type = checkSqlType( type );
    
    synchronized( param )
    {
	/*
	** Check if existing storage is compatible with requested type.
	*/
	if ( 
	     param.value == null  ||  type != param.type  ||  
	     alt != ((param.flags & PS_FLG_ALT) != 0)
	   )
	    set( param, type, alt );	// Allocate new storage

	param.flags &= ~PS_FLG_SET;	// Clear SET flag
    }
    
    return;
} // init


/*
** Name: setFlags
**
** Description:
**      Add flags to a parameter.  Does not change existing
**	flags or mark parameter as set.
**
** Input:
**      index       Parameter index (0 based).
**      flags       Parameter flags.
**
** Output:
**      None.
**
** Returns:
**      void.
**
** History:
**      15-Jun-00 (gordy)
**          Created.
**	31-Oct-02 (gordy)
**	    Add synchronization to ensure consistent update.
**	25-Feb-03 (gordy)
**	    Changed flags to short for additional bits.
*/

public void
setFlags( int index, short flags )
{
    Param param = check( index );		// Validate index & parameter.
    flags &= (~PS_FLG_SET & ~PS_FLG_ALT);	// Clear internal flags.
    
    synchronized( param ) { param.flags |= flags; }
    return;
} // setFlags


/*
** Name: setName
**
** Description:
**	Set the name of a parameter.  Does not mark parameter as set.
**
** Input:
**	index	    Parameter index (0 based).
**	name	    Parameter name.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	15-Jun-00 (gordy)
**	    Created.
*/

public void
setName( int index, String name )
{
    check( index ).name = name;
    return;
} // setName


/*
** Name: setNull
**
** Description:
**	Set a parameter to NULL.  If the parameter has not been
**	initialized for a specific SQL type, a generic NULL type
**	will be used.
**
** Input:
**	index	Parameter index.
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
*/

public void
setNull( int index )
    throws SQLException
{
    Param param = check( index );

    synchronized( param )
    {
	if ( param.value == null )
	    set( param, Types.NULL, useAltStorage( Types.NULL ) );
	
	param.value.setNull();
	param.flags |= PS_FLG_SET | dflt_flags;
    }    
    
    return;
} // setNull


/*
** Name: setBoolean
**
** Description:
**	Set parameter to a boolean value.  If the parameter has not
**	been initialized for a specific SQL type, the BOOLEAN type 
**	will be used.
**
** Input:
**	index	Parameter index.
**	value	Boolean value.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
**	15-Mar-04 (gordy)
**	    BOOLEAN now always uses alternate storage format.
*/

public void
setBoolean( int index, boolean value )
    throws SQLException
{
    Param param = check( index );

    synchronized( param )
    {
	if ( param.value == null )
	    set( param, Types.BOOLEAN, useAltStorage( Types.BOOLEAN ) );
	
	param.value.setBoolean( value );
	param.flags |= PS_FLG_SET | dflt_flags;
    }    
    
    return;
} // setBoolean


/*
** Name: setByte
**
** Description:
**	Set parameter to a byte value.  If the parameter has not
**	been initialized for a specific SQL type, the TINYINT 
**	type will be used.
**
** Input:
**	index	Parameter index.
**	value	Byte value.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
setByte( int index, byte value )
    throws SQLException
{
    Param param = check( index );

    synchronized( param )
    {
	if ( param.value == null )
	    set( param, Types.TINYINT, useAltStorage( Types.TINYINT ) );
	
	param.value.setByte( value );
	param.flags |= PS_FLG_SET | dflt_flags;
    }    
    
    return;
} // setByte


/*
** Name: setShort
**
** Description:
**	Set parameter to a short value.  If the parameter has not
**	been initialized for a specific SQL type, the SMALLINT 
**	type will be used.
**
** Input:
**	index	Parameter index.
**	value	Short value.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
setShort( int index, short value )
    throws SQLException
{
    Param param = check( index );

    synchronized( param )
    {
	if ( param.value == null )
	    set( param, Types.SMALLINT, useAltStorage( Types.SMALLINT ) );
	
	param.value.setShort( value );
	param.flags |= PS_FLG_SET | dflt_flags;
    }    
    
    return;
} // setShort


/*
** Name: setInt
**
** Description:
**	Set parameter to an int value.  If the parameter has not
**	been initialized for a specific SQL type, the INTEGER 
**	type will be used.
**
** Input:
**	index	Parameter index.
**	value	Int value.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
setInt( int index, int value )
    throws SQLException
{
    Param param = check( index );

    synchronized( param )
    {
	if ( param.value == null )
	    set( param, Types.INTEGER, useAltStorage( Types.INTEGER ) );
	
	param.value.setInt( value );
	param.flags |= PS_FLG_SET | dflt_flags;
    }    
    
    return;
} // setInt


/*
** Name: setLong
**
** Description:
**	Set parameter to a long value.  If the parameter has not
**	been initialized for a specific SQL type, the BIGINT type 
**	will be used.
**
** Input:
**	index	Parameter index.
**	value	Long value.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
**	15-Mar-04 (gordy)
**	    Implement BIGINT support.  Need to call checkSqlType() in case
**	    BIGINT not supported on connection (changes to DECIMAL).
*/

public void
setLong( int index, long value )
    throws SQLException
{
    Param param = check( index );

    synchronized( param )
    {
	if ( param.value == null )
	    set( param, checkSqlType( Types.BIGINT ), 
			useAltStorage( Types.BIGINT ) );
	
	param.value.setLong( value );
	param.flags |= PS_FLG_SET | dflt_flags;
    }    
    
    return;
} // setLong


/*
** Name: setFloat
**
** Description:
**	Set parameter to a float value.  If the parameter has not
**	been initialized for a specific SQL type, the REAL type 
**	will be used.
**
** Input:
**	index	Parameter index.
**	value	Float value.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
setFloat( int index, float value )
    throws SQLException
{
    Param param = check( index );

    synchronized( param )
    {
	if ( param.value == null )
	    set( param, Types.REAL, useAltStorage( Types.REAL ) );
	
	param.value.setFloat( value );
	param.flags |= PS_FLG_SET | dflt_flags;
    }    
    
    return;
} // setFloat


/*
** Name: setDouble
**
** Description:
**	Set parameter to a double value.  If the parameter has not
**	been initialized for a specific SQL type, the DOUBLE type 
**	will be used.
**
** Input:
**	index	Parameter index.
**	value	Double value.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
setDouble( int index, double value )
    throws SQLException
{
    Param param = check( index );

    synchronized( param )
    {
	if ( param.value == null )
	    set( param, Types.DOUBLE, useAltStorage( Types.DOUBLE ) );
	
	param.value.setDouble( value );
	param.flags |= PS_FLG_SET | dflt_flags;
    }    
    
    return;
} // setDouble


/*
** Name: setBigDecimal
**
** Description:
**	Set parameter to a BigDecimal value.  If the parameter has not
**	been initialized for a specific SQL type, the DECIMAL type 
**	will be used.
**
** Input:
**	index	Parameter index.
**	value	BigDecimal value.
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
*/

public void
setBigDecimal( int index, BigDecimal value )
    throws SQLException
{
    Param param = check( index );

    synchronized( param )
    {
	if ( param.value == null )
	    set( param, Types.DECIMAL, useAltStorage( Types.DECIMAL ) );
	
	param.value.setBigDecimal( value );
	param.flags |= PS_FLG_SET | dflt_flags;
    }    
    
    return;
} // setBigDecimal


/*
** Name: setString
**
** Description:
**	Set parameter to a String value.  If the parameter has not
**	been initialized for a specific SQL type, the VARCHAR type 
**	will be used.
**
** Input:
**	index	Parameter index.
**	value	String value.
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
*/

public void
setString( int index, String value )
    throws SQLException
{
    Param param = check( index );

    synchronized( param )
    {
	if ( param.value == null )
	        set( param, Types.VARCHAR, useAltStorage( Types.VARCHAR ) );

	param.value.setString( value );
	param.flags |= PS_FLG_SET | dflt_flags;
    }    
    
    return;
} // setString

/*
** Name: setBytes
**
** Description:
**	Set parameter to a byte array value.  If the parameter has not
**	been initialized for a specific SQL type, the VARBINARY type 
**	will be used.
**
** Input:
**	index	Parameter index.
**	value	Byte array value.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public void
setBytes( int index, byte value[] )
    throws SQLException
{
    Param param = check( index );

    synchronized( param )
    {
	if ( param.value == null )
	    set( param, Types.VARBINARY, useAltStorage( Types.VARBINARY ) );
	
	param.value.setBytes( value );
	param.flags |= PS_FLG_SET | dflt_flags;
    }    
    
    return;
} // setBytes


/*
** Name: setDate
**
** Description:
**	Set parameter to a Date value.  If the parameter has not
**	been initialized for a specific SQL type, the DATE type
**	will be used.
**
**	A relative timezone may be provided which is applied to 
**	adjust the input such that it represents the original 
**	date/time value in the timezone provided.  The default 
**	is to use the local timezone.
**
** Input:
**	index	Parameter index.
**	value	Date value.
**	tz	Relative timezone, NULL for local.
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
**	23-Jan-04 (gordy)
**	    Init to DATE (keep date/time/timestamp distinct at this level).
*/

public void
setDate( int index, Date value, TimeZone tz )
    throws SQLException
{
    Param param = check( index );

    synchronized( param )
    {
	if ( param.value == null )
	    set( param, Types.DATE, useAltStorage( Types.DATE ) );
	
	param.value.setDate( value, tz );
	param.flags |= PS_FLG_SET | dflt_flags;
    }    
    
    return;
} // setDate


/*
** Name: setTime
**
** Description:
**	Set parameter to a Time value.  If the parameter has not
**	been initialized for a specific SQL type, the TIME type 
**	will be used.
**
**	A relative timezone may be provided which is applied to 
**	adjust the input such that it represents the original 
**	date/time value in the timezone provided.  The default 
**	is to use the local timezone.
**
** Input:
**	index	Parameter index.
**	value	Time value.
**	tz	Relative timezone, NULL for local.
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
**	23-Jan-04 (gordy)
**	    Init to TIME (keep date/time/timestamp distinct at this level).
*/

public void
setTime( int index, Time value, TimeZone tz )
    throws SQLException
{
    Param param = check( index );

    synchronized( param )
    {
	if ( param.value == null )
	    set( param, Types.TIME, useAltStorage( Types.TIME ) );
	
	param.value.setTime( value, tz );
	param.flags |= PS_FLG_SET | dflt_flags;
    }    
    
    return;
} // setTime


/*
** Name: setTimestamp
**
** Description:
**	Set parameter to a Timestamp value.  If the parameter has not
**	been initialized for a specific SQL type, the TIMESTAMP type 
**	will be used.
**
**	A relative timezone may be provided which is applied to 
**	adjust the input such that it represents the original 
**	date/time value in the timezone provided.  The default 
**	is to use the local timezone.
**
** Input:
**	index	Parameter index.
**	value	Timestamp value.
**	tz	Relative timezone, NULL for local.
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
*/

public void
setTimestamp( int index, Timestamp value, TimeZone tz )
    throws SQLException
{
    Param param = check( index );

    synchronized( param )
    {
	if ( param.value == null )
	    set( param, Types.TIMESTAMP, useAltStorage( Types.TIMESTAMP ) );
	
	param.value.setTimestamp( value, tz );
	param.flags |= PS_FLG_SET | dflt_flags;
    }    
    
    return;
} // setTimestamp


/*
** Name: setBinaryStream
**
** Description:
**	Set parameter to a binary stream.  If the parameter has not
**	been initialized for a specific SQL type, the LONGVARBINARY
**	type will be used.
**
** Input:
**	index	Parameter index.
**	value	Binary stream.
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
*/

public void
setBinaryStream( int index, InputStream value )
    throws SQLException
{
    Param param = check( index );

    synchronized( param )
    {
	if ( param.value == null ) set( param, Types.LONGVARBINARY, 
					useAltStorage( Types.LONGVARBINARY ) );
	
	param.value.setBinaryStream( value );
	param.flags |= PS_FLG_SET | dflt_flags;
    }    
    
    return;
} // setBinaryStream


/*
** Name: setAsciiStream
**
** Description:
**	Set parameter to a ASCII stream.  If the parameter has not
**	been initialized for a specific SQL type, the LONGVARCHAR type 
**	will be used.
**
** Input:
**	index	Parameter index.
**	value	ASCII stream.
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
*/

public void
setAsciiStream( int index, InputStream value )
    throws SQLException
{
    Param param = check( index );

    synchronized( param )
    {
	if ( param.value == null ) set( param, Types.LONGVARCHAR, 
					useAltStorage( Types.LONGVARCHAR ) );
	
	param.value.setAsciiStream( value );
	param.flags |= PS_FLG_SET | dflt_flags;
    }    
    
    return;
} // setAsciiStream


/*
** Name: setUnicodeStream
**
** Description:
**	Set parameter to a Unicode stream.  If the parameter has not
**	been initialized for a specific SQL type, the LONGVARCHAR type 
**	will be used.
**
** Input:
**	index	Parameter index.
**	value	Unicode stream.
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
*/

public void
setUnicodeStream( int index, InputStream value )
    throws SQLException
{
    Param param = check( index );

    synchronized( param )
    {
	if ( param.value == null ) set( param, Types.LONGVARCHAR, 
					useAltStorage( Types.LONGVARCHAR ) );
	
	param.value.setUnicodeStream( value );
	param.flags |= PS_FLG_SET | dflt_flags;
    }    
    
    return;
} // setUnicodeStream


/*
** Name: setCharacterStream
**
** Description:
**	Set parameter to a character stream.  If the parameter has not
**	been initialized for a specific SQL type, the LONGVARCHAR type 
**	will be used.
**
** Input:
**	index	Parameter index.
**	value	Character stream.
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
*/

public void
setCharacterStream( int index, Reader value )
    throws SQLException
{
    Param param = check( index );

    synchronized( param )
    {
	if ( param.value == null ) set( param, Types.LONGVARCHAR, 
					useAltStorage( Types.LONGVARCHAR ) );
	
	param.value.setCharacterStream( value );
	param.flags |= PS_FLG_SET | dflt_flags;
    }    
    
    return;
} // setCharacterStream


/*
** Name: setBlob
**
** Description:
**	Set parameter to a LOB Locator.  If the parameter has not
**	been initialized for a specific SQL type, the BLOB type 
**	will be used.
**
** Input:
**	index	Parameter index.
**	value	LOB Locator.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public void
setBlob( int index, DrvBlob value )
    throws SQLException
{
    Param param = check( index );

    synchronized( param )
    {
	if ( param.value == null )
	    set( param, Types.BLOB, useAltStorage( Types.BLOB ) );

	param.value.setBlob( value );
	param.flags |= PS_FLG_SET | dflt_flags;
    }

    return;
} // setBlob


/*
** Name: setClob
**
** Description:
**	Set parameter to a LOB Locator.  If the parameter has not
**	been initialized for a specific SQL type, the CLOB type
**	will be used.
**
** Input:
**	index	Parameter index.
**	value	LOB Locator.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public void
setClob( int index, DrvClob value )
    throws SQLException
{
    Param param = check( index );

    synchronized( param )
    {
	if ( param.value == null )  set( param, Types.CLOB, true );
	param.value.setClob( value );
	param.flags |= PS_FLG_SET | dflt_flags;
    }

    return;
} // setClob


/*
** Name: setClob
**
** Description:
**	Set parameter to a LOB Locator.  If the parameter has not
**	been initialized for a specific SQL type, the CLOB type
**	will be used.
**
** Input:
**	index	Parameter index.
**	value	LOB Locator.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public void
setClob( int index, DrvNlob value )
    throws SQLException
{
    Param param = check( index );

    synchronized( param )
    {
	if ( param.value == null )  set( param, Types.CLOB, false );
	param.value.setClob( value );
	param.flags |= PS_FLG_SET | dflt_flags;
    }

    return;
} // setClob


/*
** Name: setObject
**
** Description:
**	Set a parameter to a Java object value.  If the parameter
**	has not been initialized for a specific SQL type, the SQL
**	type associated with the class of the Java object will be
**	used.
**
** Input:
**	index	Parameter index.
**	value	Java object.
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
**	23-Jan-04 (gordy)
**	    Added coerce() method to handle coercions which are not
**	    provided directly by the storage classes.
**	15-Mar-04 (gordy)
**	    Need to call checkSqlType() in case object type is aliased.
*/

public void
setObject( int index, Object value )
    throws SQLException
{
    Param param = check( index );

    synchronized( param )
    {
	if ( param.value == null )
	{
	    int type = SqlData.getSqlType( value );
	    set( param, checkSqlType( type ), useAltStorage( type ) );
	}
	
	param.value.setObject( value );
	param.flags |= PS_FLG_SET | dflt_flags;
    }    
    
    return;
} // setObject

/*
** Name: setObject
**
** Description:
**	Set a parameter to a Java object value.  If the parameter 
**	has not been initialized for a specific SQL type, the input 
**	type will be used.
**
** Input:
**	index	Parameter index.
**	value	Java object.
**	type 	Parameter type.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	28-Apr-04 (wansh01)
**	    Created.
**	19-Jun-06 (gordy)
**	    Pass alt indicator to coerce();
*/

public void
setObject( int index, Object value, int type )
    throws SQLException
{
    Param param = check( index );

    type = checkSqlType( type );
    synchronized( param )
    {
	if ( param.value == null )
	    set( param, type, useAltStorage( type ) );
	
 
	param.value.setObject( coerce( value, type, 
				       (param.flags & PS_FLG_ALT) != 0 ) );
	param.flags |= PS_FLG_SET | dflt_flags;
    }    
    
    return;
} // setObject


/*
** Name: setObject
**
** Description:
**	Set a parameter to a Java object value.  If the parameter 
**	has not been initialized for a specific SQL type, the input 
**	type will be used.
**
** Input:
**	index	Parameter index.
**	value	Java object.
**	type 	Parameter type.
**	scale	Number of digits following decimal point.
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
**	23-Jan-04 (gordy)
**	    Added coerce() method to handle coercions which are not
**	    provided directly by the storage classes.
**	15-Mar-04 (gordy)
**	    Need to call checkSqlType() in case object type is aliased.
**	28-Apr-04 (wansh01)
**	    Added type as an input parameter.The input type is used   
** 	    for init and coerce(). 	
**	19-Jun-06 (gordy)
**	    Pass alt indicator to coerce();
*/

public void
setObject( int index, Object value, int type, int scale )
    throws SQLException
{
    Param param = check( index );

    type = checkSqlType( type );
    synchronized( param )
    {
	if ( param.value == null )
	    set( param, type, useAltStorage( type ) );

  	param.value.setObject( coerce( value, type,
				       (param.flags & PS_FLG_ALT) != 0 ),
			       scale );
	param.flags |= PS_FLG_SET | dflt_flags;
    }    
    
    return;
} // setObject


/*
** Name: set
**
** Description:
**	Sets the parameter type, allocates storage based on type
**	and alternate storage format, and sets ALT flag according 
**	to input.  Other flags and parameter name are not changed.
**
**	Caller must provide synchronization on parameter.
**
** Input:
**	param	Parameter.
**	type	SQL type.
**	alt	True for alternate storage format.
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
*/

private void
set( Param param, int type, boolean alt )
    throws SQLException
{
    short flags = (short)(param.flags & ~PS_FLG_ALT);	// All but ALT
    if ( alt )  flags |= PS_FLG_ALT;
    
    param.type = type;
    param.value = getStorage( type, alt );
    param.flags = flags;
    return;
} // set


/*
** Name: check
**
** Description:
**	Check the parameter index and expand the parameter
**	array if necessary.  The referenced parameter is
**	(optionally) allocated.
**
** Input:
**	index	Parameter index (0 based).
**
** Output:
**	None.
**
** Returns:
**	Param	Referenced parameter.	    
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Returns the parameter object.  Rather than iterating the
**	    expansion size simply calculate from current setting/request.
**	    Don't need to clear pre-existing parameters (see clear()).
*/

private synchronized Param
check( int index )
{
    if ( index < 0 )  throw new IndexOutOfBoundsException();

    /*
    ** If the new index is beyond the current limit, expand 
    ** the parameter array to include the new index.
    */
    if ( index >= params.length )
    {
	int new_max = params.length + EXPAND_SIZE;
	if ( index >= new_max )  new_max = index + EXPAND_SIZE;
	Param new_array[] = new Param[ new_max ];
	System.arraycopy( params, 0, new_array, 0, params.length );
	params = new_array;
    }

    /*
    ** Make sure the referenced parameter is allocated
    ** and that it is included in the current set.
    */
    if ( params[ index ] == null )  params[ index ] = new Param();
    if ( param_cnt <= index )  param_cnt = index + 1;
    return( params[ index ] );
} // check


/*
** Name: useAltStorage
**
** Description:
**	Determine if the alternate storage format should be used
**	for a given SQL type.
**
** Input:
**	sqlType	    Target SQL type.
**
** Output:
**	None.
**
** Returns:
**	boolean	    TRUE if alternate storage should be used.
**
** History:
**	25-Feb-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Moved to ParamSet to support updatable result-sets.
**	15-Mar-04 (gordy)
**	    Implemented BIGINT support, default to DECIMAL when not supported.
**	    BOOLEAN now implemented as always using alternate storage format.
**	19-Jun-06 (gordy)
**	    ANSI Date/Time data type support.  ANSI is date/time primay
**	    storage format and Ingres date is alternate.
**	10-Feb-09 (rajus01) SIR 121238
**	    Added new SQL data types NCHAR, NVARCHAR, and LONGNVARCHAR.
**	12-Mar-09 (gordy)
**	    CLOB has alternate storage depending on UCS2 support.
**	26-Oct-09 (gordy)
**	    BOOLEAN supported at API protocol level 6.
**	30-Jul-10 (gordy)
**	    Add config flag to force boolean to integer.
**	    Convert config item snd_ing_dte to config flag.
*/

private boolean
useAltStorage( int sqlType )
{
    switch( sqlType )
    {
    /*
    ** BOOLEAN is supported starting with API protocol level 6.
    ** Alternate storage format is TINYINT.
    */
    case Types.BIT :
    case Types.BOOLEAN :	
	return( conn.db_protocol_level < DBMS_API_PROTO_6  ||
		(conn.cnf_flags & conn.CNF_INTBOOL) != 0 );
    
    /*
    ** BIGINT is supported starting with API protocol level 3.
    ** The preferred alternate storage format is DECIMAL, but
    ** DECIMAL has it's own alternate format.  BIGINT uses the
    ** same alternate format as DECIMAL and is forced to DECIMAL
    ** when not supported.
    */
    case Types.BIGINT :	
	if ( conn.db_protocol_level >= DBMS_API_PROTO_3 )
	     return( false );
	
	/*
	** Fall through to utilize DECIMAL alternate logic...
	*/
	
    /*
    ** The messaging protocol indicates that DECIMAL values
    ** will be converted to VARCHAR automatically if not 
    ** supported by the DBMS.  This works well with Ingres, 
    ** (which supports char to numeric coercion), but does
    ** not work with gateways.  The alternate storage format
    ** is DOUBLE, which is used for non-Ingres servers which
    ** don't support DECIMAL.
    */
    case Types.NUMERIC :
    case Types.DECIMAL :	
	return( ! conn.is_ingres  &&  
		conn.db_protocol_level < DBMS_API_PROTO_1 );

    /*
    ** For character types, standard storge is Unicode while
    ** alternate storage is host character set.
    */
    case Types.CHAR :
    case Types.VARCHAR :
    case Types.LONGVARCHAR :
    case Types.NCHAR :
    case Types.NVARCHAR :
    case Types.LONGNVARCHAR :	
    case Types.CLOB :
    case Types.NCLOB :
	return( ! conn.ucs2_supported );

    /*
    ** For date/time types, standard storage is ANSI while
    ** alternate storage is Ingres date.
    */
    case Types.DATE :
    case Types.TIME :
    case Types.TIMESTAMP :
	return( conn.db_protocol_level < DBMS_API_PROTO_4  || 
		(conn.cnf_flags & conn.CNF_INGDATE) != 0 );
    }

    return( false );
} // useAltStorage


/*
** Name: getStorage
**
** Description:
**	Allocates storage for SQL data values base on the SQL type
**	and optional alternate storage format.  The class of the
**	object returned is defined in the class description.
**
** Input:
**	sqlType		SQL type.
**	alt		True for alternate storage format.
**
** Output:
**	None.
**
** Returns:
**	SqlData		SQL data value.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
**	19-Jun-06 (gordy)
**	    ANSI Date/Time data type support.  IngresDate is now
**	    the alternate storage for DATE/TIME/TIMESTAMP and
**	    SqlDate/SqlTime/SqlTimestamp are the primary storage.
**	15-Nov-06 (gordy)
**	    Added support for LOB Locators via BLOB/CLOB.
**	 9-Sep-08 (gordy)
**	    Set max decimal precision.
**	24-Dec-08 (gordy)
**	    Use date/time formatters associated with connection.
**	02-Feb-09 (rajus01) SIR 121238
**	    Restored the lost changes as a result of my previous
**	    code submission for JDBC 4.0 project.
**      27-Oct-10 (rajus01) SIR 124588, SD issue 147074
**	    Added ability to store blank date back in the DBMS.
**	    Use the configured empty_date replacement value for
**	    IngresDate.
*/

private SqlData
getStorage( int sqlType, boolean alt )
    throws SQLException
{
    SqlData sqlData;
    
    switch( sqlType )
    {
    case Types.NULL :		sqlData = new SqlNull();	break;
    case Types.TINYINT :	sqlData = new SqlTinyInt();	break;
    case Types.SMALLINT :	sqlData = new SqlSmallInt();	break;
    case Types.INTEGER :	sqlData = new SqlInt();		break;
    case Types.REAL :		sqlData = new SqlReal();	break;
    case Types.DOUBLE :		sqlData = new SqlDouble();	break;
    case Types.BINARY :		sqlData = new SqlByte();	break;
    case Types.VARBINARY :	sqlData = new SqlVarByte();	break;
    case Types.LONGVARBINARY :	sqlData = new SqlLongByte();	break;
    case Types.BLOB :		sqlData = new SqlBLoc( conn );	break;

    case Types.CLOB :
	sqlData = alt ? (SqlData)(new SqlCLoc( conn ))
		      : (SqlData)(new SqlNLoc( conn ));
	break;
    
    case Types.BOOLEAN :
	sqlData = alt ? (SqlData)(new SqlTinyInt()) 
		      : (SqlData)(new SqlBool());
	break;
	
    case Types.BIGINT :	
	sqlData = alt ? (SqlData)(new SqlDouble()) 
		      : (SqlData)(new SqlBigInt());
	break;
	
    case Types.DECIMAL :
	sqlData = alt ? (SqlData)(new SqlDouble()) 
		      : (SqlData)(new SqlDecimal( conn.max_dec_prec ));
	break;

    case Types.CHAR :
	sqlData = alt ? (SqlData)(new SqlChar( msg.getCharSet() ))
		      : (SqlData)(new SqlNChar());
	break;
	
    case Types.VARCHAR :
	sqlData = alt ? (SqlData)(new SqlVarChar( msg.getCharSet() ))
		      : (SqlData)(new SqlNVarChar());
	break;
	
    case Types.LONGVARCHAR :
	sqlData = alt ? (SqlData)(new SqlLongChar( msg.getCharSet() )) 
		      : (SqlData)(new SqlLongNChar());
	break;
    
    case Types.DATE :
	sqlData = alt ? (SqlData)(new IngresDate( conn.dt_frmt, 
						  conn.osql_dates,
						  conn.timeValuesInGMT(),
						  conn.cnf_empty_date ))
		      : (SqlData)(new SqlDate( conn.dt_frmt ));
	break;
	
    case Types.TIME :
	sqlData = alt ? (SqlData)(new IngresDate( conn.dt_frmt,
						  conn.osql_dates,
						  conn.timeValuesInGMT(),
						  conn.cnf_empty_date ))
		      : (SqlData)(new SqlTime( conn.dt_frmt,
			conn.osql_dates ? DBMS_TYPE_TMWO : DBMS_TYPE_TMTZ ));
	break;
	
    case Types.TIMESTAMP :	
	sqlData = alt ? (SqlData)(new IngresDate( conn.dt_frmt,
						  conn.osql_dates,
						  conn.timeValuesInGMT(),
						  conn.cnf_empty_date ))
		      : (SqlData)(new SqlTimestamp( conn.dt_frmt,
			conn.osql_dates ? DBMS_TYPE_TSWO : DBMS_TYPE_TSTZ ));
	break;
	
    default :
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
    }
    
    return( sqlData );
} // getStorage


/*
** Name: checkSqlType
**
** Description:
**	Validates that a SQL type is supported as a parameter type
**	by this class.  Returns the supported parameter type after
**	applying alias translations.
**
** Input:
**	sqlType	    SQL type to validate.
**
** Output:
**	None.
**
** Returns:
**	int	    Supported SQL type.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	10-May-01 (gordy)
**	    CHAR no longer forced to VARCHAR (character arrays fully 
**	    supported), BINARY to VARBINARY (same storage format, but 
**	    different transport format at JDBC Server), LONGVARCHAR
**	    and LONGVARBINARY to VARCHAR and VARBINARY (IO classes
**	    now supported).  NULL is now permitted.
**	19-Feb-03 (gordy)
**	    BIT now alias for BOOLEAN.
**	 1-Dec-03 (gordy)
**	    Alias BOOLEAN and BIGINT until supported by messaging protocol.
**	15-Mar-04 (gordy)
**	    BIGINT supported at DBMS/API protocol level 3.  Force back to
**	    DECIMAL when not supported.  Method can no longer be static
**	    due to need to check protocol level.  BIT is aliased to BOOLEAN.
**	    BOOLEAN is also now supported (but always as alternate format).
**	15-Nov-06 (gordy)
**	    Added support for LOB Locators via BLOB/CLOB.
**	10-Feb-09 (rajus01) SIR 121238
**	    Added new SQL types NCLOB, NCHAR, NVARCHAR, and LONGNVARCHAR.
*/

private int
checkSqlType( int sqlType )
    throws SQLException
{
    switch( sqlType )
    {
    /*
    ** Aliases
    */
    case Types.BIT :		return( Types.BOOLEAN );
    case Types.FLOAT :		return( Types.DOUBLE );
    case Types.NUMERIC :	return( Types.DECIMAL );
    case Types.NCHAR :    	return( Types.CHAR );
    case Types.NVARCHAR :	return( Types.VARCHAR );
    case Types.LONGNVARCHAR :	return( Types.LONGVARCHAR );
    case Types.NCLOB :    	return( Types.CLOB );

    /*
    ** BIGINT is supported starting at DBMS/API protocol level 3.
    ** Otherwise, force to DECIMAL (BIGINT and DECIMAL use the
    ** same alternate storage format to accomodate the change).
    */
    case Types.BIGINT :		
	return( (conn.db_protocol_level >= DBMS_API_PROTO_3) 
		? Types.BIGINT : Types.DECIMAL );
    
    /*
    ** The following types are OK.
    */
    case Types.NULL :
    case Types.BOOLEAN :
    case Types.TINYINT :
    case Types.SMALLINT :
    case Types.INTEGER :
    case Types.REAL :
    case Types.DOUBLE :
    case Types.DECIMAL :
    case Types.BINARY :
    case Types.VARBINARY :
    case Types.LONGVARBINARY :
    case Types.CHAR :
    case Types.VARCHAR :
    case Types.LONGVARCHAR :
    case Types.DATE :
    case Types.TIME :
    case Types.TIMESTAMP :	
    case Types.BLOB :
    case Types.CLOB :
	return( sqlType );
    }
    
    /*
    ** All others are not allowed.
    */
    throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
} // checkSqlType


/*
** Name: coerce
**
** Description:
**	Applies required JDBC coercions to parameter values which
**	are not directly supported by the underlying storage classes.
**	Returns the original input if no coercion is applicable.
**	Does not enforce any coercion restrictions.
**
** Input:
**	obj	Source parameter value.
**	type	Target SQL type.
**	alt	Is alternate storage being used.
**
** Output:
**	None.
**
** Returns:
**	Object	Coerced value (original value if no coercion).
**
** History:
**	23-Jan-04 (gordy)
**	    Created.
**	20-May-04 (gordy)
**	    Allow for empty strings to support Ingres empty dates.
**	19-Jun-06 (gordy)
**	    ANSI Date/Time data type support.  Ingres dates now
**	    used as alternate storage format.
*/

private static Object
coerce( Object obj, int type, boolean alt )
    throws SQLException
{
    int src_type = SqlData.getSqlType( obj );

    /*
    ** Currently, the only coercions not supported by the underlying
    ** storage class involve date/time values.  This is due to the
    ** use of a single storage class for date/time values to represent
    ** the Ingres date datatype.  The IngresDate storage class handles
    ** assignment based on the input type, so our work simply entails
    ** getting the input to match the target type.
    */
    switch( type )
    {
    case Types.DATE :
	/*
	** IngresDate only used as aternate storage format.
	*/
	if ( ! alt )  break;

	/*
	** IngresDate converts Date objects to date-only values,
	** Timestamp objects to timestamp values, and strings 
	** to whichever format they represent.  Need to coerce 
	** Timestamp objects to date-only values and restrict
	** string conversions to just dates.  JDBC does not 
	** require coercion of Time values.
	*/
	switch( src_type )
	{
	case Types.CHAR :
	    /*
	    ** Permit empty strings (will be handled by the
	    ** storage class) even though technically they
	    ** are not valid DATE values.
	    */
	    if ( ((char[])obj).length == 0 )  break;
	    
	    try { obj = Date.valueOf( String.copyValueOf( (char[])obj ) ); }
	    catch( Exception ex )
	    { throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }
	    break;
	    
	case Types.VARCHAR :
	    /*
	    ** Permit empty strings (will be handled by the
	    ** storage class) even though technically they
	    ** are not valid DATE values.
	    */
	    if ( ((String)obj).length() == 0 )  break;
	    
	    try { obj = Date.valueOf( (String)obj ); }
	    catch( Exception ex )
	    { throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }
	    break;
	    
	case Types.TIMESTAMP :
	    obj = new Date( ((Timestamp)obj).getTime() );
	    break;
	}
	break;
    
    case Types.TIME :
	/*
	** IngresDate only used as aternate storage format.
	*/
	if ( ! alt )  break;

	/*
	** IngresDate converts Time objects to time values,
	** Timestamp objects to timestamp values, and strings
	** to whichever format they represent.  Need to coerce
	** Timestamp objects to time values and restrict string
	** conversions to just times.  JDBC does not require
	** coercion of Date values.  
	*/
	switch( src_type )
	{
	case Types.CHAR :
	    /*
	    ** Permit empty strings (will be handled by the
	    ** storage class) even though technically they
	    ** are not valid TIME values.
	    */
	    if ( ((char[])obj).length == 0 )  break;
	    
	    try { obj = Time.valueOf( String.copyValueOf( (char[])obj ) ); }
	    catch( Exception ex )
	    { throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }
	    break;
	    
	case Types.VARCHAR :
	    /*
	    ** Permit empty strings (will be handled by the
	    ** storage class) even though technically they
	    ** are not valid TIME values.
	    */
	    if ( ((String)obj).length() == 0 )  break;
	    
	    try { obj = Time.valueOf( (String)obj ); }
	    catch( Exception ex )
	    { throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }
	    break;
	    
	case Types.TIMESTAMP :
	    obj = new Time( ((Timestamp)obj).getTime() );
	    break;
	}
	break;
    
    case Types.TIMESTAMP :
	/*
	** IngresDate only used as aternate storage format.
	*/
	if ( ! alt )  break;

	/*
	** IngresDate converts Timestamp objects to timestamp
	** values, Date objects to date-only values, time
	** objects to time values, and strings to whichever
	** format they represent.  Need to coerce Date objects
	** to timestamp values, Time objects to timestamp
	** values, and restrict string conversions to just
	** timestamps.  
	*/
	switch( src_type )
	{
	case Types.CHAR :
	    /*
	    ** Permit empty strings (will be handled by the
	    ** storage class) even though technically they
	    ** are not valid TIMESTAMP values.
	    */
	    if ( ((char[])obj).length == 0 )  break;
	    
	    try { obj = Timestamp.valueOf( String.copyValueOf((char[])obj) ); }
	    catch( Exception ex )
	    { throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }
	    break;
	    
	case Types.VARCHAR :
	    /*
	    ** Permit empty strings (will be handled by the
	    ** storage class) even though technically they
	    ** are not valid TIMESTAMP values.
	    */
	    if ( ((String)obj).length() == 0 )  break;
	    
	    try { obj = Timestamp.valueOf( (String)obj ); }
	    catch( Exception ex )
	    { throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }
	    break;

	case Types.DATE :
	    obj = new Timestamp( ((Date)obj).getTime() );
	    break;
	    
	case Types.TIME :
	    obj = new Timestamp( ((Date)obj).getTime() );
	    break;
	}
	break;
    }
    
    return( obj );
} // coerce


/*
** Name: sendDesc
**
** Description:
**	Send a DESC message for current set of parameters.
**
** Input:
**	eog	Is this message end-of-group?
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	17-Dec-99 (gordy)
**	    Conversion to BLOB now determined by DBMS max varchar len.
**	 2-Feb-00 (gordy)
**	    Send short streams as VARCHAR or VARBINARY.
**	 7-Feb-00 (gordy)
**	    For the NULL datatype or NULL data values, send the generic
**	    NULL datatype and the nullable indicator.
**	13-Jun-00 (gordy)
**	    Nullable byte converted to flags, parameter names supported
**	    for database procedures (EdbcCall).
**	15-Nov-00 (gordy)
**	    Extracted Param class to create a stand-alone Paramset class.
**	 8-Jan-01 (gordy)
**	    Parameter set passed as parameter to support batch processing.
**	10-May-01 (gordy)
**	    Now pass DBMS type to support alternate formats (UCS2 for now).
**	    Character arrays also supported.
**	 1-Jun-01 (gordy)
**	    Removed conversion of BLOB to fixed length type for short lengths.
**      13-Aug-01 (loera01) SIR 105521
**          Set the JDBC_DSC_GTT flag if the parameter is a global temp
**          table.
**	20-Feb-02 (gordy)
**	    Decimal not supported by gateways at proto level 0.  Send as float.
**	 6-Sep-02 (gordy)
**	    Use formatted length of CHAR and VARCHAR parameters rather than
**	    character length.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	19-Feb-03 (gordy)
**	    Replaced BIT with BOOLEAN.  Skip unset parameters (default
**	    procedure parameters).  ALT character parameters now stored
**	    in communication format.
**	24-June-03 (wansh01)
**	    for binary type with 0 length send as varbinary.
**	 1-Dec-03 (gordy)
**	    Moved to ParamSet to support updatable result sets.
**	23-Jan-04 (gordy)
**	    Set NULL CHAR/BINARY columns to non-zero length to avoid coercion.
**	15-Mar-04 (gordy)
**	    BIGINT now full implemented with same alternate format as DECIMAL
**	    (type is forced to DECIMAL when not supported).  BOOLEAN should
**	    always be alternate, but force to alternate just in case.
**	29-Apr-04 (wansh01)
**	    To calculate prec for a Decimal, prec is one less if long  
**	    value of the decimal is zero and scale is more than 0. 
**	    i.e. 0.12345 prec is 5. 
**	28-May-04 (gordy)
**	    Add max column lengths for NCS columns.
**	11-Aug-04 (gordy)
**	    The described length of NCS columns need to be in bytes.
**	28-Jun-06 (gordy)
**	    OUT procedure parameter support.
**	22-Sep-06 (gordy)
**	    Determine scale of TIMESTAMP values when fractional
**	    seconds are present.
**	15-Nov-06 (gordy)
**	    Added support for LOB Locators via BLOB/CLOB.
**	12-Mar-09 (gordy)
**	    Null LOB values sent as more compatible variable length type.
**	26-Oct-09 (gordy)
**	    Boolean now fully supported.
**       9-Nov-10 (gordy & rajus01) Bug 124712
**          Re-worked conversion of BigDecimal precision and scale into
**          the appropriate Ingres precision and scale. 
*/

public synchronized void
sendDesc( boolean eog )
    throws SQLException
{
    /*
    ** Count the number of paramters which are set.
    */
    short set_cnt = 0;

    for( int param = 0; param < param_cnt; param++ )
	if ( params[ param ] != null  &&  
	     (params[ param ].flags & PS_FLG_SET) != 0 )
	    set_cnt++;

    msg.begin( MSG_DESC );
    msg.write( set_cnt );
    
    for( int param = 0; param < param_cnt; param++ )
    {
	if ( params[ param ] == null  ||
	     (params[ param ].flags & PS_FLG_SET) == 0 )  
	    continue;		// Skip unset params

	SqlData	value = params[ param ].value;
	short	type = (short)params[ param ].type;
	short	pflags = params[ param ].flags;
	boolean	alt = (pflags & PS_FLG_ALT) != 0;

	short	dbms = 0;
	int	length = 0;
	byte	prec = 0;
	byte	scale = 0;
	byte	flags = 0;

	switch( type )
	{
	case Types.NULL :
	    dbms = DBMS_TYPE_LONG_TEXT;
	    flags |= MSG_DSC_NULL;
	    break;

	case Types.BOOLEAN :
	    if ( ! alt )
		dbms = DBMS_TYPE_BOOL;
	    else
	    {
		type = (short)Types.TINYINT;
		dbms = DBMS_TYPE_INT;
		length = 1;
	    }
	    break;
	
	case Types.TINYINT :
	    dbms = DBMS_TYPE_INT;
	    length = 1;
	    break;

	case Types.SMALLINT :
	    dbms = DBMS_TYPE_INT;
	    length = 2;
	    break;

	case Types.INTEGER :
	    dbms = DBMS_TYPE_INT;
	    length = 4;
	    break;

	case Types.BIGINT :
	    if ( alt )
	    {
		type = (short)Types.DOUBLE;
		dbms = DBMS_TYPE_FLOAT;
		length = 8;
	    }
	    else
	    {
		dbms = DBMS_TYPE_INT;
		length = 8;
	    }
	    break;

	case Types.REAL :
	    dbms = DBMS_TYPE_FLOAT;
	    length = 4;
	    break;

	case Types.DOUBLE :
	    dbms = DBMS_TYPE_FLOAT;
	    length = 8;
	    break;

	case Types.DECIMAL :
	    if ( alt )
	    {
		type = (short)Types.DOUBLE;
		dbms = DBMS_TYPE_FLOAT;
		length = 8;
	    }
	    else
	    {
		dbms = DBMS_TYPE_DECIMAL;
		prec = 15;

		if ( ! value.isNull() )
		{
		    BigDecimal  dec = value.getBigDecimal();

		    /*
 		    ** In BigDecimal, precision is the number of digits in
 		    ** the numeric portion of the value, excluding leading
 		    ** 0's (even following the decimal point).  Scale is a
 		    ** combination of the position of the decimal point and
 		    ** any exponent.
 		    */
		    prec = (byte)dec.precision();
		    scale = (byte)dec.scale();
		     
 		    /*
 		    ** In Ingres decimals, precision is the total number of
 		    ** possible digits or size of the value and only
 		    ** excludes leading 0's preceding the decimal point.
 		    ** Scale is simply the position of the decimal point.
 		    **
 		    ** If the BigDecimal scale is negative, then the decimal
 		    ** point must be moved to the right with a corresponding
 		    ** increase in precision.
 		    */
 		    if ( scale < 0 )
 		    {
 			prec += -scale;
 			scale = 0;
 		    }
 
 		    /*
 		    ** Precision must be large enough to hold scale portion.
 		    */
 		    if ( prec < scale )  prec = scale;
		}
	    }
	    break;
	
	case Types.DATE :
	    if ( ! alt )
		dbms = DBMS_TYPE_ADATE;
	    else
	    {
		type = (short)Types.TIMESTAMP;
		dbms = DBMS_TYPE_IDATE;
	    }
	    break;

	case Types.TIME :
	    if ( ! alt )
		dbms = conn.osql_dates ? DBMS_TYPE_TMWO : DBMS_TYPE_TMTZ;
	    else
	    {
		type = (short)Types.TIMESTAMP;
		dbms = DBMS_TYPE_IDATE;
	    }
	    break;

	case Types.TIMESTAMP :
	    if ( alt )
		dbms = DBMS_TYPE_IDATE;
	    else
	    {
		dbms = conn.osql_dates ? DBMS_TYPE_TSWO : DBMS_TYPE_TSTZ;
		int nanos = ((SqlTimestamp)value).getNanos();

		/*
		** Determine scale if non-zero fractional seconds component.
		*/
		if ( nanos > 0 )
		{
		    /*
		    ** Scale of nano-seconds is 9 but 
		    ** decreases for each trailing zero.
		    */
		    scale = 9;
		    String frac = Integer.toString( nanos );

		    for( int i = frac.length() - 1; i >= 0; i-- )
			if ( frac.charAt( i ) == '0' )
			    scale--;
			else
			    break;
		}
	    }
	    break;

	case Types.CHAR :
	    /*
	    ** Determine length in characters.  
	    ** NULL values assume {n}char(1).
	    */
	    if ( value.isNull() )
	        length = 1;
	    else  if ( alt )
		length = ((SqlChar)value).length();
	    else
		length = ((SqlNChar)value).length();

	    /*
	    ** Long character arrays are sent as BLOBs.
	    ** Zero length arrays must be sent as VARCHAR.
	    ** UCS2 used when supported unless alternate 
	    ** format is requested.
	    */
	    if ( length <= 0 )
	    {
		type = (short)Types.VARCHAR;
		dbms = alt ? DBMS_TYPE_VARCHAR : DBMS_TYPE_NVARCHAR;
	    }
	    else  if ( alt  &&  length <= conn.max_char_len )
		dbms = DBMS_TYPE_CHAR;
	    else  if ( ! alt  &&  length <= conn.max_nchr_len )
	    {
		dbms = DBMS_TYPE_NCHAR;
		length *= 2;	// Convert to bytes
	    }
	    else  
	    {
		type = (short)Types.LONGVARCHAR;
		dbms = alt ? DBMS_TYPE_LONG_CHAR : DBMS_TYPE_LONG_NCHAR;
		length = 0;
	    }
	    break;

	case Types.VARCHAR :	
	    /*
	    ** Determine length in characters.  
	    ** NULL values assume {n}varchar(1).
	    */
	    if ( value.isNull() )
		length = 1;
	    else  if ( alt )
		length = ((SqlVarChar)value).length();
	    else
		length = ((SqlNVarChar)value).length();

	    /*
	    ** Long strings are sent as BLOBs.  UCS2 used 
	    ** when supported unless alternate format is 
	    ** requested.
	    */
	    if ( alt  &&  length <= conn.max_vchr_len )
		dbms = DBMS_TYPE_VARCHAR;
	    else  if ( ! alt  &&  length <= conn.max_nvch_len )
	    {
		dbms = DBMS_TYPE_NVARCHAR;
		length *= 2;	// Convert to bytes
	    }
	    else
	    {
		type = (short)Types.LONGVARCHAR;
		dbms = alt ? DBMS_TYPE_LONG_CHAR : DBMS_TYPE_LONG_NCHAR;
		length = 0;
	    }
	    break;

	case Types.LONGVARCHAR :
	    /*
	    ** NULL values sent as more compatible varchar(1) type.
	    ** UCS2 used when supported unless alternate format is 
	    ** requested.
	    */
	    if ( ! value.isNull() )
		dbms = alt ? DBMS_TYPE_LONG_CHAR : DBMS_TYPE_LONG_NCHAR;
	    else
	    {
		dbms = alt ? DBMS_TYPE_VARCHAR : DBMS_TYPE_NVARCHAR;
		length = 1;
	    }
	    break;

	case Types.BINARY :
	    /*
	    ** Determine length.  NULL values assume byte(1).
	    */
	    length = value.isNull() ? 1 : ((SqlByte)value).length();

	    /*
	    ** Long arrays are sent as BLOBs.  Zero length 
	    ** arrays must be sent as VARBINARY.
	    */
	    if ( length <= 0 ) 
	    {
		type = (short)Types.VARBINARY;
		dbms = DBMS_TYPE_VARBYTE;
	    }
	    else  if (  length <= conn.max_byte_len )
		dbms = DBMS_TYPE_BYTE;
	    else  
	    {
		type = (short)Types.LONGVARBINARY;
		dbms = DBMS_TYPE_LONG_BYTE;
		length = 0;
	    }
	    break;

	case Types.VARBINARY :
	    /*
	    ** Determine length.  NULL values assume varbyte(1).
	    */
	    length = value.isNull() ? 1 : ((SqlVarByte)value).length();

	    /*
	    ** Long arrays are sent as BLOBs.
	    */
	    if ( length <= conn.max_vbyt_len )
		dbms = DBMS_TYPE_VARBYTE;
	    else
	    {
		type = (short)Types.LONGVARBINARY;
		dbms = DBMS_TYPE_LONG_BYTE;
		length = 0;
	    }
	    break;

	case Types.LONGVARBINARY :
	    /*
	    ** NULL values sent as more compatible varbyte(1) type.
	    */
	    if ( ! value.isNull() )
		dbms = DBMS_TYPE_LONG_BYTE;
	    else
	    {
		dbms = DBMS_TYPE_VARBYTE;
		length = 1;
	    }
	    break;

	case Types.BLOB :
	    /*
	    ** NULL values sent as more compatible varbyte(1) type.
	    */
	    type = Types.LONGVARBINARY;

	    if ( ! value.isNull() )
		dbms = DBMS_TYPE_LBLOC;
	    else
	    {
		dbms = DBMS_TYPE_VARBYTE;
		length = 1;
	    }
	    break;

	case Types.CLOB :
	    /*
	    ** NULL values sent as more compatible varchar(1) type.
	    ** UCS2 used when supported unless alternate format is 
	    ** requested.
	    */
	    type = Types.LONGVARCHAR;

	    if ( ! value.isNull() )
		dbms = alt ? DBMS_TYPE_LCLOC : DBMS_TYPE_LNLOC;
	    else
	    {
		dbms = alt ? DBMS_TYPE_VARCHAR : DBMS_TYPE_NVARCHAR;
		length = 1;
	    }
	    break;
	}

	/*
	** Translate flags.
	*/
	if ( value.isNull() )  flags |= MSG_DSC_NULL;

        switch( pflags & PS_FLG_PROC_IO )
	{
	case PS_FLG_PROC_IN :	flags |= MSG_DSC_PIN;	break;
	case PS_FLG_PROC_IO :	flags |= MSG_DSC_PIO;	break;

	case PS_FLG_PROC_OUT :	
	    /*
	    ** Since a generic NULL value is provided for output
	    ** parameters, handle as INOUT when OUT isn't supported.
	    */
	    flags |= (conn.msg_protocol_level >= MSG_PROTO_5) ? MSG_DSC_POUT 
							      : MSG_DSC_PIO;
	    break;
	}

	if ( (pflags & PS_FLG_PROC_GTT) != 0 )  
	    flags |= MSG_DSC_GTT;

	msg.write( (short)type );
	msg.write( (short)dbms );
	msg.write( (short)length );
	msg.write( (byte)prec );
	msg.write( (byte)scale );
	msg.write( (byte)flags );
	
	if ( params[ param ].name == null )
	    msg.write( (short)0 );	// no name.
	else
	    msg.write( params[ param ].name );
    }

    msg.done( eog );
    return;
} // sendDesc


/*
** Name: sendData
**
** Description:
**	Send a DATA message for current set of parameters.
**
** Input:
**	eog	Is this message end-of-group?
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	25-Mar-10 (gordy)
**	    Converted to cover for method which takes explicit header flags.
*/

public void
sendData( boolean eog )
    throws SQLException
{ sendData( eog ? MSG_HDR_EOG : 0 ); }


/*
** Name: sendData
**
** Description:
**	Send a DATA message for current set of parameters.
**
** Input:
**	flags	Message header flags (MSG_HDR_EOG,MSG_HDR_EOB).
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Implemented support for BLOBs.
**	12-Nov-99 (gordy)
**	    Use configured date formatter.
**	17-Dec-99 (gordy)
**	    Conversion to BLOB now determined by DBMS max varchar len.
**	 2-Feb-00 (gordy)
**	    Send short streams as VARCHAR or VARBINARY.
**	15-Nov-00 (gordy)
**	    Extracted Param class to create a stand-alone Paramset class.
**	 8-Jan-01 (gordy)
**	    Parameter set passed as parameter to support batch processing.
**	10-May-01 (gordy)
**	    Support UCS2 as alternate format for character types.  Char
**	    arrays fully supported as separate BINARY and VARBINARY types.
**	 1-Jun-01 (gordy)
**	    Removed conversion of BLOB to fixed length type for short lengths.
**	20-Feb-02 (gordy)
**	    Decimal not supported by gateways at proto level 0.  Send as float.
**	 6-Sep-02 (gordy)
**	    Use formatted length of CHAR and VARCHAR parameters rather than
**	    character length.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.  
**	19-Feb-03 (gordy)
**	    Replaced BIT with BOOLEAN.  Skip unset parameters (default
**	    procedure parameters).  ALT character parameters now stored
**	    in communication format.
**	26-Jun-03 (gordy)
**	    Missed an ALT storage class.
**	27-June-03 (wansh01)
**	    Handle binary type with 0 length as varbinary. 	
**	 1-Dec-03 (gordy)
**	    Moved to ParamSet to support updatable result sets.
**	23-Jan-04 (gordy)
**	    Properly handle zero length NCHAR columns using emptyNVarChar.
**	    Set NULL CHAR/BINARY columns to non-zero length to avoid coercion
**	    (zero length values were being sent instead of mulls).
**	15-Mar-04 (gordy)
**	    BIGINT now supported.  BOOLEAN should always be in alternate
**	    storage format (force to alternate until BOOLEAN supported).
**	28-May-04 (gordy)
**	    Add max column lengths for NCS columns.
**	19-Jun-06 (gordy)
**	    ANSI Date/Time data type support.  Ingres date is now
**	    the alternate storage for DATE/TIME/TIMESTAMP and
**	    SqlDate/SqlTime/SqlTimestamp are the primary storage.
**	15-Nov-06 (gordy)
**	    Added support for LOB Locators via BLOB/CLOB.
**	26-Oct-09 (gordy)
**	    BOOLEAN type now fully supported.
**	25-Mar-10 (gordy)
**	    Changed parameter to header flags to support optional EOB.
*/

public synchronized void
sendData( byte flags )
    throws SQLException
{
    msg.begin( MSG_DATA );
    
    for( int param = 0; param < param_cnt; param++ )
    {
	if ( params[ param ] == null  ||
	     (params[ param ].flags & PS_FLG_SET) == 0 )  
	    continue;		// Skip unset params.

	SqlData	value = params[ param ].value;
	short	type = (short)params[ param ].type;
	boolean	alt = ((params[ param ].flags & PS_FLG_ALT) != 0);
	int	length;

	switch( type )
	{
	case Types.NULL :	msg.write( (SqlNull)value );		break;
	case Types.TINYINT :    msg.write( (SqlTinyInt)value );		break;
	case Types.SMALLINT :   msg.write( (SqlSmallInt)value );	break;
	case Types.INTEGER :    msg.write( (SqlInt)value );		break;
	case Types.REAL :	msg.write( (SqlReal)value );		break;
	case Types.DOUBLE :	msg.write( (SqlDouble)value );		break;

	case Types.DATE :
	    if ( alt )
		msg.write( (IngresDate)value );
	    else
		msg.write( (SqlDate)value );
	    break;

	case Types.TIME :
	    if ( alt )
		msg.write( (IngresDate)value );
	    else
		msg.write( (SqlTime)value );
	    break;

	case Types.TIMESTAMP :	
	    if ( alt )
		msg.write( (IngresDate)value );
	    else
		msg.write( (SqlTimestamp)value );
	    break;

	case Types.BOOLEAN :
	    if ( alt )
		msg.write( (SqlTinyInt)value );
	    else
		msg.write( (SqlBool)value );
	    break;
	    
	case Types.BIGINT :
	    if ( alt )
		msg.write( (SqlDouble)value );
	    else
		msg.write( (SqlBigInt)value );
	    break;

	case Types.DECIMAL :
	    if ( alt )
		msg.write( (SqlDouble)value );
	    else
		msg.write( (SqlDecimal)value );
	    break;

	case Types.CHAR :
	    /*
	    ** Determine length.  NULL values assume {n}char(1).
	    */
	    if ( value.isNull() )
		length = 1;
	    else  if ( alt )
		length = ((SqlChar)value).length();
	    else
		length = ((SqlNChar)value).length();

	    /*
	    ** Zero length strings must be sent as VARCHAR.
	    ** Long strings are sent as BLOBs.  UCS2 used 
	    ** when supported.
	    */
	    if ( alt )
	    {
		if ( length <= 0 )
		    msg.write( emptyVarChar );
		else  if ( length <= conn.max_char_len )
		    msg.write( (SqlChar)value );
		else
		{
		    if ( tempLongChar == null )  
		        tempLongChar = new SqlLongChar( msg.getCharSet() );
		    tempLongChar.set( (SqlChar)value );
		    msg.write( tempLongChar );
		}
	    }
	    else
	    {
		if ( length <= 0 )
		    msg.write( emptyNVarChar );
		else  if ( length <= conn.max_nchr_len )
		    msg.write( (SqlNChar)value );
		else
		{
		    if (tempLongNChar == null) 
			tempLongNChar = new SqlLongNChar();
		    tempLongNChar.set( (SqlNChar)value );
		    msg.write( tempLongNChar );
		}
	    }
	    break;

	case Types.VARCHAR :	
	    /*
	    ** Determine length.  NULL values assume {n}varchar(1).
	    */
	    if ( value.isNull() )
		length = 1;
	    else  if ( alt )
		length = ((SqlVarChar)value).length();
	    else
		length = ((SqlNVarChar)value).length();

	    /*
	    ** Long strings are sent as BLOBs.  
	    ** UCS2 used when supported.
	    */
	    if ( alt )
	    {
		if ( length <= conn.max_vchr_len )
		    msg.write( (SqlVarChar)value );
		else  
		{
		    if ( tempLongChar == null )  
		        tempLongChar = new SqlLongChar( msg.getCharSet() );
		    tempLongChar.set( (SqlVarChar)value );
		    msg.write( tempLongChar );
		}
	    }
	    else
	    {
		if ( length <= conn.max_nvch_len )
		    msg.write( (SqlNVarChar)value );
		else
		{
		    if (tempLongNChar == null) 
			tempLongNChar = new SqlLongNChar();
		    tempLongNChar.set( (SqlNVarChar)value );
		    msg.write( tempLongNChar );
		}
	    }
	    break;

	case Types.LONGVARCHAR :
	    /*
	    ** UCS2 used when supported.
	    */
	    if ( alt )
		msg.write( (SqlLongChar)value );
	    else
		msg.write( (SqlLongNChar)value );
	    break;

	case Types.BINARY :
	    /*
	    ** Determine length.  NULL values assume byte(1).
	    */
	    length = value.isNull() ? 1 : ((SqlByte)value).length();
	    
	    /*
	    ** Long arrays are sent as BLOBs.  Zero length 
	    ** arrays must be sent as VARBINARY.
	    ** 
	    */
	    if ( length <= 0 )
		msg.write( emptyVarByte );
	    else  if ( length <= conn.max_byte_len )
		msg.write( (SqlByte)value );
	    else
	    {
		if ( tempLongByte == null )  tempLongByte = new SqlLongByte();
		tempLongByte.set( (SqlByte)value );
		msg.write( tempLongByte );
	    }
	    break;

	case Types.VARBINARY :
	    /*
	    ** Determine length.  NULL values assume varbyte(1).
	    */
	    length = value.isNull() ? 1 : ((SqlVarByte)value).length();
	    
	    /*
	    ** Long arrays are sent as BLOBs.
	    */
	    if ( length <= conn.max_vbyt_len )	
		msg.write( (SqlVarByte)value );
	    else
	    {
		if ( tempLongByte == null )  tempLongByte = new SqlLongByte();
		tempLongByte.set( (SqlVarByte)value );
		msg.write( tempLongByte );
	    }
	    break;

	case Types.LONGVARBINARY :
	    msg.write( (SqlLongByte)value );
	    break;

	case Types.BLOB :
	    msg.write( (SqlLoc)value );
	    break;

	case Types.CLOB :
	    msg.write( (SqlLoc)value );
	    break;
	}
    }

    msg.done( flags );
    return;
} // sendData


/*
** Name: Param
**
** Description:
**	Class representing an entry in a parameter data-set.
**
**  Public data
**
**	type	    SQL type.
**	value	    SQL data value.
**	flags	    Flags.
**	name	    Name.
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
**	11-Jan-01 (gordy)
**	    This class is Cloneable since parameter object
**	    values are immutable.
**	25-Feb-03 (gordy)
**	    Changed flags to short for additional bits.
**	 1-Dec-03 (gordy)
**	    Use OTHER as default type since NULL is valid setting.
**	    Value now stored as SqlData object.  Removed clone() and
**	    added constructors and set().
*/

private static class
Param
{

    public  int		type = Types.OTHER;
    public  SqlData	value = null;
    public  short	flags = 0;
    public  String	name = null;

    
/*
** Name: Param
**
** Description:
**	Class constructor for default parameter.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public
Param()
{
} // Param


/*
** Name: Param
**
** Description:
**	Class constructor for initialized parameter.
**
** Input:
**	type	SQL type.
**	value	SQL data storage.
**	short	Flags.
**	name	Name.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public
Param( int type, SqlData value, short flags, String name )
{
    this.type = type;
    this.value = value;
    this.flags = flags;
    this.name = name;
} // Param


/*
** Name: clear
**
** Description:
**	Reset parameter fields to initial settings.
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
**	15-Nov-00 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    OTHER is now the default type.
*/

public synchronized void
clear()
{
    type = Types.OTHER;
    value = null;
    flags = 0;
    name = null;
    return;
} // clear


/*
** Name: set
**
** Description:
**	Set parameter fields.
**
** Input:
**	type	SQL type.
**	value	SQL data storage.
**	short	Flags.
**	name	Name.
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
*/

public synchronized void
set( int type, SqlData value, short flags, String name )
{
    this.type = type;
    this.value = value;
    this.flags = flags;
    this.name = name;
    return;
} // set


} // class Param

} // class ParamSet
