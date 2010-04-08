/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbc;

/*
** Name: ParamSet
**
** Description:
**	Implements the EDBC helper class ParamSet which manages
**	a parameter data set.
**
**  Classes:
**
**	ParamSet    A parameter data set.
**
**  Private Classes:
**
**	Param	    A parameter entry in the data set.
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
*/


import	java.math.BigDecimal;
import	java.text.DateFormat;
import	java.io.InputStream;
import	java.io.ByteArrayInputStream;
import	java.io.Reader;
import	java.io.CharArrayReader;
import	java.io.StringReader;
import	java.io.IOException;
import	java.sql.Types;
import	java.sql.Date;
import	java.sql.Time;
import	java.sql.Timestamp;
import	java.sql.SQLException;
import	ca.edbc.util.EdbcEx;


/*
** Name: ParamSet
**
** Description:
**	Class which manages a parameter data set.
**
**  Public Methods:
**
**	clone		    Make a copy of this parameter set.
**	getCount	    Returns number of parameters.
**	clear		    Clear all parameter settings.
**	setDefaultFlags	    Set default parameter flags.
**	isSet		    Has a parameter been set.
**	setObject	    Set parameter to Java object.
**	set		    Set parameter type and value.
**	setFlags	    Set parameter flags.
**	setName		    Set parameter name.
**	getType		    Returns parameter type.
**	getValue	    Returns parameter value.
**	getFlags	    Returns parameter flags.
**	getName		    Returns parameter name.
**	getSqlType	    Determine parameter type from value.
**
**  Private Data:
**
**	EXPAND_SIZE	    Size to grow the parameter data array.
**	max_vchr_len	    Max varchar string length.
**	param_flags	    Default parameter flags.
**	param_cnt	    Number of actual parameters.
**	params		    Parameter array.
**
**  Private Methods:
**
**	check		    Validate parameter index.
**	checkSqlType	    Validate/convert parameter type.
**	coerce		    Validate/convert parameter value.
**	str2bin		    Convert hex string to byte array.
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
*/

class
ParamSet
    implements EdbcErr
{

    /*
    ** Constants for parameter flags.
    */
    public static final byte	PS_FLG_ALT	    = 0x01;
    public static final byte	PS_FLG_PROC_PARAM   = 0x02;
    public static final byte	PS_FLG_PROC_BYREF   = 0x04;
    public static final byte	PS_FLG_PROC_GTT     = 0x08;

    /*
    ** While a generic Vector class is a possible
    ** implementation for this class, a parameter
    ** data set is best represented by a sparse
    ** array which is not well suited to the Vector
    ** class.
    */
    private static final int	EXPAND_SIZE = 10;
    private int			param_cnt = 0;
    private Param		params[];

    private DateFormat		dateFormatter;
    private byte		param_flags = 0;	// Default flags.
    private int			max_vchr_len = -1;	// Max string length.


/*
** Name: ParamSet
**
** Description:
**	Class constructor.  Creates a parameter set of the
**	default size.  A date formatter is required for
**	converting date/time objects into internal format.
**
** Input:
**	df	Connection date formatter.
**
** Output:
**	None.
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
*/

public
ParamSet( DateFormat df )
{
    this( 0, df );
} // ParamSet


/*
** Name: ParamSet
**
** Description:
**	Class constructor.  Creates a parameter set of the
**	requested size.  If the provided size is 0 or less,
**	a parameter set of the default size is created.  A 
**	date formatter is required for converting date/time 
**	objects into internal format.
**
** Input:
**	size	Initial size.
**	df	Connection date formatter.
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
*/

public
ParamSet( int size, DateFormat df )
{
    if ( size <= 0 )  size = EXPAND_SIZE;
    params = new Param[ size ];
    dateFormatter = df;
} // ParamSet


/*
** Name: clone
**
** Description:
**	Create a copy of this parameter set.  This class is not
**	Cloneable at the field level since changes in individual
**	parameters should remain local.  The actual set of params
**	is individually cloned to localize changes.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	ParamSet	Copy of this parameter set.
**
** History:
**	11-Jan-01 (gordy)
**	    Created.
*/

public Object
clone()
{
    ParamSet	ps = new ParamSet( param_cnt, dateFormatter );

    ps.param_cnt = param_cnt;
    ps.param_flags = param_flags;
    ps.max_vchr_len = max_vchr_len;

    for( int i = 0; i < param_cnt; i++ )
	if ( params[ i ] != null )
	    ps.params[ i ] = (Param)params[ i ].clone();

    return( ps );
} // clone


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
*/

public void
clear( boolean purge )
{
    if ( purge )
	for( int i = 0; i < param_cnt; i++ )
	    params[ i ] = null;

    param_cnt = 0;
    return;
} // clear


/*
** Name: setMaxLength
**
** Description:
**	Set the maximum string length.  Longer strings will
**	be truncated.  Short BLOBs will be converted to strings.
**
** Input:
**	length	    Max string length.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 8-Jan-01 (gordy)
**	    Created.
*/

public void
setMaxLength( int length )
{
    max_vchr_len = length;
    return;
} // setMaxLength


/*
** Name: setDefaultFlags
**
** Description:
**	Set the default parameter flags which are assigned
**	when a parameter is assigned using the set() method.
**
** Input:
**	flags	    Descriptor flags.
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
setDefaultFlags( byte flags )
{
    param_flags = flags;
    return;
} // setDefaultFlags


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
*/

public boolean
isSet( int index )
{
    return( index < param_cnt  &&  params[ index ] != null );
} // isSet


/*
** Name: setObject
**
** Description:
**	Set parameter to a generic Java object with the requested
**	type and scale.  
**	
**	The input object must be one of the classes supported by
**	getSqlType().  The SQL type may get aliased to a corresponding
**	type as required by the set() method.  The input object will
**	be coerced, if necessary, to match the resulting type.  Scale
**	is only used if the SQL type is DECIMAL.  A scale value of -1 
**	indicates that the scale of the value should be used.
**
** Input:
**	index		Parameter index (0 based).
**	value		Java object.
**	sqlType		Target SQL type.
**	scale		Number of digits following decimal point, or -1.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 8-Jan-01 (gordy)
**	    Created.
*/

public void
setObject( int index, Object value, int sqlType, int scale )
    throws SQLException
{
    /*
    ** Validate requested type and standardize (resolve aliases).
    */
    sqlType = checkSqlType( sqlType );

    /*
    ** Convert to desired type and/or scale.
    */
    try { value = coerce( value, sqlType, scale ); }
    catch( EdbcEx ex )
    {
        throw ex;
    }
    catch( Exception e )
    { 
	String error = e.getMessage();
	throw EdbcEx.get( E_JD0006_CONVERSION_ERR ); 
    }

    set( index, sqlType, value );
    return;
} // setObject


/*
** Name: set
**
** Description:
**	Save a parameter type and value.  No conversions are
**	performed on the type or value, but the relationship
**	between value class and SQL type must correspond to
**	the following table.
**
**	SQL Type		    Object Class
**	-------------		    --------------------
**	NULL			    null
**	BIT			    java.lang.Boolean
**	TINYINT			    java.lang.Byte
**	SMALLINT		    java.lang.Short
**	INTEGER			    java.lang.Integer
**	BIGINT			    java.lang.Long
**	REAL			    java.lang.Float
**	DOUBLE			    java.lang.Double
**	DECIMAL			    java.math.BigDecimal
**	CHAR			    char[]
**	VARCHAR			    java.lang.String
**	LONGVARCHAR		    Reader
**	BINARY			    byte[]
**	VARBINARY		    byte[]
**	LONGVARBINARY		    InputStream
**	DATE			    java.lang.String 
**	TIME			    java.lang.String
**	TIMESTAMP		    java.lang.String
**
**	In addition, alias substitution should be done for
**	the following: FLOAT -> DOUBLE, NUMERIC -> DECIMAL.
**
**	DATE, TIME, and TIMESTAMP values must be converted 
**	to Strings before being saved.
**
** Input:
**	index	    Parameter index (0 based).
**	type	    SQL type.
**	value	    Parameter value.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	19-May-99 (gordy)
**	    Created.
*/

public void
set( int index, int type, Object value )
{
    check( index );
    params[ index ].type = type;
    params[ index ].value = value;
    params[ index ].flags |= param_flags;
    return;
} // set


/*
** Name: setFlags
**
** Description:
**      Add flags to a parameter.
**
** Input:
**      index       Parameter index (0 based).
**      flags       Descriptor flags.
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
*/

public void
setFlags( int index, byte flags )
{
    check( index );
    params[ index ].flags |= flags;
    return;
} // setFlags


/*
** Name: setName
**
** Description:
**	Set the name of a parameter.
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
    check( index );
    params[ index ].name = name;
    return;
} // setName


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
*/

public int
getType( int index )
{
    check( index );
    return( params[ index ].type );
} // getType


/*
** Name: getValue
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
**	Object	Parameter value.
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
*/

public Object
getValue( int index )
{
    check( index );
    return( params[ index ].value );
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
**	byte	Parameter flags.
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
*/

public byte
getFlags( int index )
{
    check( index );
    return( params[ index ].flags );
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
*/

public String
getName( int index )
{
    check( index );
    return( params[ index ].name );
} // getName


/*
** Name: getSqlType
**
** Description:
**	Assigns a unique SQL type for each class of Java object
**	supported as an input parameter.  Null values return the
**	generic SQL NULL type.  
**
**	While the JDBC interface does not directly support char 
**	arrays (favoring Strings instead), this object type is
**	supported through the generic object methods.  Since we 
**	must also support Strings and streams, char arrays are 
**	given the SQL type of CHAR and Strings are given the SQL 
**	type of VARCHAR.  
**
**	Byte arrays are given the SQL type of VARBINARY and there
**	is no current class which maps to BINARY.  LONGVARCHAR 
**	is used for Reader objects while LONGVARBINARY is used 
**	for InputStream objects.  While the stream methods can 
**	map an InputStream to LONGVARCHAR by creating a Reader 
**	translator, this method is restricted by the input class.
**
**	Object Class		    SQL Type
**	------------		    --------
**	null			    NULL
**	java.lang.Boolean	    BIT
**	java.lang.Byte		    TINYINT
**	java.lang.Short		    SMALLINT
**	java.lang.Integer	    INTEGER
**	java.lang.Long		    BIGINT
**	java.lang.Float		    REAL
**	java.lang.Double	    DOUBLE
**	java.math.BigDecimal	    DECIMAL
**	char[]			    CHAR
**	java.lang.String	    VARCHAR
**	Reader			    LONGVARCHAR
**	byte[]			    VARBINARY
**	InputStream		    LONGVARBINARY
**	java.sql.Date		    DATE
**	java.sql.Time		    TIME
**	java.sql.Timestamp	    TIMESTAMP
**
** Input:
**	obj	Generic Java object.
**
** Output:
**	None.
**
** Returns:
**	int	SQL type.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	10-May-01 (gordy)
**	    Allow sub-classing by using instanceof instead of class ID.
**	    Support IO classes for BLOBs.
*/

public int
getSqlType( Object obj )
    throws SQLException
{
    if ( obj == null )			return( Types.NULL );
    if ( obj instanceof Boolean )	return( Types.BIT );
    if ( obj instanceof Byte )		return( Types.TINYINT );
    if ( obj instanceof Short )		return( Types.SMALLINT );
    if ( obj instanceof Integer )	return( Types.INTEGER );
    if ( obj instanceof Long )		return( Types.BIGINT );
    if ( obj instanceof Float )		return( Types.REAL );
    if ( obj instanceof Double )	return( Types.DOUBLE );
    if ( obj instanceof BigDecimal )	return( Types.DECIMAL );
    if ( obj instanceof String )	return( Types.VARCHAR );
    if ( obj instanceof Reader )	return( Types.LONGVARCHAR );
    if ( obj instanceof InputStream )	return( Types.LONGVARBINARY );
    if ( obj instanceof Date )		return( Types.DATE );
    if ( obj instanceof Time )		return( Types.TIME );
    if ( obj instanceof Timestamp )	return( Types.TIMESTAMP );

    Class oc = obj.getClass();
    if ( oc.isArray() )
    {
	if ( oc.getComponentType() == Character.TYPE )
	    return( Types.CHAR );
	
	if ( oc.getComponentType() == Byte.TYPE )  
	    return( Types.VARBINARY );
    }

    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
} // getSqlType


/*
** Name: check
**
** Description:
**	Check the parameter index and expand the parameter
**	array if necessary.  The referenced parameter is
**	(optionally) allocated.
**
** Input:
**	index	    Parameter index (0 based).
**
** Output:
**	None.
**
** Returns:
**	int	    
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
*/

private synchronized void
check( int index )
{
    int	    new_max;

    if ( index < 0 )  throw new IndexOutOfBoundsException();

    /*
    ** If the new index is beyond the current limit,
    ** expand the parameter array to include the new
    ** index.
    */
    for( new_max = params.length; index >= new_max; new_max += EXPAND_SIZE );

    if ( new_max > params.length )
    {
	Param new_array[] = new Param[ new_max ];
	System.arraycopy( params, 0, new_array, 0, params.length );
	params = new_array;
    }

    /*
    ** Any pre-allocated parameters between the previous
    ** upper limit and the new limit are cleared and the
    ** parameter count updated.  Allocate the parameter
    ** for the current index if needed.
    */
    for( ; param_cnt <= index; param_cnt++ )
	if ( params[ param_cnt ] != null )  params[ param_cnt ].clear();

    if ( params[ index ] == null )  params[ index ] = new Param();

    return;
} // check


/*
** Name: checkSqlType
**
** Description:
**	Validates a SQL type as being supported by the driver
**	and translates the type to the corresponding type used
**	internally when saving the parameter.
**
** Input:
**	sqlType	    SQL type to validate.
**
** Output:
**	None.
**
** Returns:
**	int	    Parameter SQL type.
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
*/

private int
checkSqlType( int sqlType )
    throws EdbcEx
{
    switch( sqlType )
    {
    /*
    ** Aliases
    */
    case Types.FLOAT :		sqlType = Types.DOUBLE;	    break;
    case Types.NUMERIC :	sqlType = Types.DECIMAL;    break;

    /*
    ** The following types are OK.
    */
    case Types.NULL :
    case Types.BIT :
    case Types.TINYINT :
    case Types.SMALLINT :
    case Types.INTEGER :
    case Types.BIGINT :
    case Types.REAL :
    case Types.DOUBLE :
    case Types.DECIMAL :
    case Types.CHAR :
    case Types.VARCHAR :
    case Types.LONGVARCHAR :
    case Types.BINARY :
    case Types.VARBINARY :
    case Types.LONGVARBINARY :
    case Types.DATE :
    case Types.TIME :
    case Types.TIMESTAMP :
	break;

    /*
    ** All others are not allowed.
    */
    default :
	throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
    }

    return( sqlType );
} // checkSqlType


/*
** Name: coerce
**
** Description:
**	Coerce an input parameter value to a type selected by
**	the application and supported by the driver.  The input
**	object must be of a class supprted by getSqlType().  The 
**	target type must be of a supported type as determined by 
**	the set() method (use checkSqlType() to validate).
**
** Input:
**	obj	    The input object.
**	sqlType	    Target type validated by checkSqlType().
**	scale	    Scale for decimal values.
**
** Output:
**	None.
**
** Returns:
**	Object	    The object (matching sqlType) to be saved.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	 1-Nov-00 (gordy)
**	    Date types are now saved as a string.  Throw Exception
**	    if coercion from objType to sqlType not supported.
**	11-Apr-01 (gordy)
**	    Ingres supports (to some extent) dates without time,
**	    so do not include time with DATE datatypes.
**	10-May-01 (gordy)
**	    Removed objType parameter to enforce supported object classes.
**	    IO streams now supported but require wrapper class, so added
**	    to classes need identity coercions.  Char arrays now fully
**	    supported.  Cleaned up permitted coercions compared to spec.
**	    Character to binary coercions should use hex conversion
**	    (? same as is done in EdbcRslt).
**	 1-Jun-01 (gordy)
**	    Reader/InputStream no longer need wrapper classes.
**      12-Feb-04 (rajus01) Bug #111769 Startrack #EDJDBC82
**     	    An invalid Timestamp value causes the JDBC server abort the
**          client connection. Validate the Timestamp string before 
**          passing it to JDBC server.
**      08-Jun-07 (weife01) Bug #118474
**      JDBC client fails with "An invalid datatype conversion was 
**      requested." when the involved timestamp was not synchronized. 
**          
*/

private Object
coerce( Object obj, int sqlType, int scale )
    throws Exception
{
    int objType = getSqlType( obj );	// Determine input type.
    
    /*
    ** A null reference is returned for a null input
    ** or a request to coerce to NULL.
    */
    if ( objType == Types.NULL  ||  sqlType == Types.NULL )  
	return( null );
    
    /*
    ** Return the input object for identity coercions
    ** (but not for a few special cases).
    */
    if ( objType == sqlType )
	switch( objType )
	{
	case Types.DECIMAL :		// Convert scale.
	case Types.DATE :		// Convert to strings.
	case Types.TIME :
	case Types.TIMESTAMP :
	    break;
	
	default :			// No conversion required.
	    return( obj );
	}

    /*
    ** Handle specific coercions.
    */
    switch( sqlType )
    {
    case Types.BIT :
	switch( objType )
	{
	case Types.TINYINT :
	    return( new Boolean( ((Byte)obj).byteValue() != 0 ) );
	case Types.SMALLINT :
	    return( new Boolean( ((Short)obj).shortValue() != 0 ) );
	case Types.INTEGER :
	    return( new Boolean( ((Integer)obj).intValue() != 0 ) );
	case Types.BIGINT :
	    return( new Boolean( ((Long)obj).longValue() != 0L ) );
	case Types.REAL :
	    return( new Boolean( ((Float)obj).floatValue() != 0.0 ) );
	case Types.DOUBLE :
	    return( new Boolean( ((Double)obj).doubleValue() != 0.0 ) );
	case Types.DECIMAL :
	    return( new Boolean( ((BigDecimal)obj).signum() != 0 ) );
	case Types.CHAR :
	    return( new Boolean( String.copyValueOf( (char[])obj) ) );
	case Types.VARCHAR :
	    return( new Boolean( (String)obj ) );
	}
	break;

    case Types.TINYINT :
	switch( objType )
	{
	case Types.BIT :
	    return( new Byte( (byte)
			      ( ((Boolean)obj).booleanValue() ? 1 : 0 ) ) );
	case Types.SMALLINT :
	    return( new Byte( ((Short)obj).byteValue() ) );
	case Types.INTEGER :
	    return( new Byte( ((Integer)obj).byteValue() ) );
	case Types.BIGINT :
	    return( new Byte( ((Long)obj).byteValue() ) );
	case Types.REAL :
	    return( new Byte( ((Float)obj).byteValue() ) );
	case Types.DOUBLE :
	    return( new Byte( ((Double)obj).byteValue() ) );
	case Types.DECIMAL :
	    return( new Byte( ((BigDecimal)obj).byteValue() ) );
	case Types.CHAR :
	    return( new Byte( String.copyValueOf( (char[])obj ) ) );
	case Types.VARCHAR :
	    return( new Byte( (String)obj ) );
	}
	break;

    case Types.SMALLINT :
	switch( objType )
	{
	case Types.BIT :
	    return( new Short( (short)
			       ( ((Boolean)obj).booleanValue() ? 1 : 0 ) ) );
	case Types.TINYINT :
	    return( new Short( ((Byte)obj).shortValue() ) );
	case Types.INTEGER :
	    return( new Short( ((Integer)obj).shortValue() ) );
	case Types.BIGINT :
	    return( new Short( ((Long)obj).shortValue() ) );
	case Types.REAL :
	    return( new Short( ((Float)obj).shortValue() ) );
	case Types.DOUBLE :
	    return( new Short( ((Double)obj).shortValue() ) );
	case Types.DECIMAL :
	    return( new Short( ((BigDecimal)obj).shortValue() ) );
	case Types.CHAR :
	    return( new Short( String.copyValueOf( (char[])obj ) ) );
	case Types.VARCHAR :
	    return( new Short( (String)obj ) );
	}
	break;

    case Types.INTEGER :
	switch( objType )
	{
	case Types.BIT :
	    return( new Integer( (int)
				 ( ((Boolean)obj).booleanValue() ? 1 : 0 ) ) );
	case Types.TINYINT :
	    return( new Integer( ((Byte)obj).intValue() ) );
	case Types.SMALLINT :
	    return( new Integer( ((Short)obj).intValue() ) );
	case Types.BIGINT :
	    return( new Integer( ((Long)obj).intValue() ) );
	case Types.REAL :
	    return( new Integer( ((Float)obj).intValue() ) );
	case Types.DOUBLE :
	    return( new Integer( ((Double)obj).intValue() ) );
	case Types.DECIMAL :
	    return( new Integer( ((BigDecimal)obj).intValue() ) );
	case Types.CHAR :
	    return( new Integer( String.copyValueOf( (char[])obj ) ) );
	case Types.VARCHAR :
	    return( new Integer( (String)obj ) );
	}
	break;

    case Types.BIGINT :
	switch( objType )
	{
	case Types.BIT :
	    return( new Long( (long)
			      ( ((Boolean)obj).booleanValue() ? 1 : 0 ) ) );
	case Types.TINYINT :
	    return( new Long( ((Byte)obj).longValue() ) );
	case Types.SMALLINT :
	    return( new Long( ((Short)obj).longValue() ) );
	case Types.INTEGER :
	    return( new Long( ((Integer)obj).longValue() ) );
	case Types.REAL :
	    return( new Long( ((Float)obj).longValue() ) );
	case Types.DOUBLE :
	    return( new Long( ((Double)obj).longValue() ) );
	case Types.DECIMAL :
	    return( new Long( ((BigDecimal)obj).longValue() ) );
	case Types.CHAR :
	    return( new Long( String.copyValueOf( (char[])obj ) ) );
	case Types.VARCHAR :
	    return( new Long( (String)obj ) );
	}
	break;

    case Types.REAL :
	switch( objType )
	{
	case Types.BIT :
	    return( new Float( (float)
			       ( ((Boolean)obj).booleanValue() ? 1 : 0 ) ) );
	case Types.TINYINT :
	    return( new Float( ((Byte)obj).floatValue() ) );
	case Types.SMALLINT :
	    return( new Float( ((Short)obj).floatValue() ) );
	case Types.INTEGER :
	    return( new Float( ((Integer)obj).floatValue() ) );
	case Types.BIGINT :
	    return( new Float( ((Long)obj).floatValue() ) );
	case Types.DOUBLE :
	    return( new Float( ((Double)obj).floatValue() ) );
	case Types.DECIMAL :
	    return( new Float( ((BigDecimal)obj).floatValue() ) );
	case Types.CHAR :
	    return( new Float( String.copyValueOf( (char[])obj ) ) );
	case Types.VARCHAR :
	    return( new Float( (String)obj ) );
	}
	break;

    case Types.DOUBLE :
	switch( objType )
	{
	case Types.BIT :
	    return( new Double( (double)
			        ( ((Boolean)obj).booleanValue() ? 1 : 0 ) ) );
	case Types.TINYINT :
	    return( new Double( ((Byte)obj).doubleValue() ) );
	case Types.SMALLINT :
	    return( new Double( ((Short)obj).doubleValue() ) );
	case Types.INTEGER :
	    return( new Double( ((Integer)obj).doubleValue() ) );
	case Types.BIGINT :
	    return( new Double( ((Long)obj).doubleValue() ) );
	case Types.REAL :
	    return( new Double( ((Float)obj).doubleValue() ) );
	case Types.DECIMAL :
	    return( new Double( ((BigDecimal)obj).doubleValue() ) );
	case Types.CHAR :
	    return( new Double( String.copyValueOf( (char[])obj ) ) );
	case Types.VARCHAR :
	    return( new Double( (String)obj ) );
	}
	break;

    case Types.DECIMAL :
	BigDecimal dec = null;

	switch( objType )
	{
	case Types.BIT :
	    dec = BigDecimal.valueOf( (long)(((Boolean)obj).booleanValue() ? 1 : 0) );
	    break;
	case Types.TINYINT :
	    dec = BigDecimal.valueOf( ((Byte)obj).longValue() );
	    break;
	case Types.SMALLINT :
	    dec = BigDecimal.valueOf( ((Short)obj).longValue() );
	    break;
	case Types.INTEGER :
	    dec = BigDecimal.valueOf( ((Integer)obj).longValue() );
	    break;
	case Types.BIGINT :
	    dec = BigDecimal.valueOf( ((Long)obj).longValue() );
	    break;
	case Types.REAL :
	    dec = new BigDecimal( ((Float)obj).doubleValue() );
	    break;
	case Types.DOUBLE :
	    dec = new BigDecimal( ((Double)obj).doubleValue() );
	    break;
	case Types.DECIMAL :	// Adjusting scale.
	    dec = (BigDecimal)obj;
	    break;
	case Types.CHAR :
	    dec = new BigDecimal( String.copyValueOf( (char[])obj ) );
	    break;
	case Types.VARCHAR :
	    dec = new BigDecimal( (String)obj );
	    break;
	}

	if ( dec != null )
	{
	    if ( scale >= 0  &&  scale != dec.scale() )  
		dec = dec.setScale( scale, BigDecimal.ROUND_HALF_UP );
	    return( dec );
	}
	break;

    case Types.CHAR :
	switch( objType )
	{
	case Types.BIT :
	case Types.TINYINT :
	case Types.SMALLINT :
	case Types.INTEGER :
	case Types.BIGINT :
	case Types.REAL :
	case Types.DOUBLE :
	case Types.DECIMAL :
	case Types.DATE :
	case Types.TIME :
	case Types.TIMESTAMP :
	    return( obj.toString().toCharArray() );
	case Types.VARCHAR :
	    return( ((String)obj).toCharArray() );
	}
	break;

    case Types.VARCHAR :
	switch( objType )
	{
	case Types.BIT :
	case Types.TINYINT :
	case Types.SMALLINT :
	case Types.INTEGER :
	case Types.BIGINT :
	case Types.REAL :
	case Types.DOUBLE :
	case Types.DECIMAL :
	case Types.DATE :
	case Types.TIME :
	case Types.TIMESTAMP :
	    return( obj.toString() );
	case Types.CHAR :
	    return( new String( (char[])obj ) );
	}
	break;

    case Types.LONGVARCHAR :
	switch( objType )
	{
	case Types.BIT :
	case Types.TINYINT :
	case Types.SMALLINT :
	case Types.INTEGER :
	case Types.BIGINT :
	case Types.REAL :
	case Types.DOUBLE :
	case Types.DECIMAL :
	case Types.DATE :
	case Types.TIME :
	case Types.TIMESTAMP :
	    return( new StringReader( obj.toString() ) );

	case Types.CHAR :
	    return( new CharArrayReader( (char[])obj ) );

	case Types.VARCHAR :
	    return( new StringReader( (String)obj ) );
	}
	break;

    case Types.BINARY :
    case Types.VARBINARY :
	switch( objType )
	{
	/*
	** When the source type is character based, a hex to binary
	** conversion is done (opposite of getString( VARBINARY )).
	*/
	case Types.CHAR : 
	    return( str2bin( new String( (char[])obj ) ) );
	case Types.VARCHAR :
	    return( str2bin( (String)obj ) );
	case Types.BINARY :
	case Types.VARBINARY :
	    return( obj );
	}
	break;

    case Types.LONGVARBINARY :
	switch( objType )
	{
	case Types.CHAR :
	    return( new ByteArrayInputStream(
			str2bin( new String( (char[])obj ) ) ) );

	case Types.VARCHAR :
	    return( new ByteArrayInputStream( str2bin( (String)obj ) ) );

	case Types.BINARY :
	case Types.VARBINARY :
	    return( new ByteArrayInputStream( (byte[])obj ) );
	}
	break;

    case Types.DATE :
	Date	date = null;

	switch( objType )
	{
	case Types.CHAR :
	    date = Date.valueOf( String.copyValueOf( (char[])obj ) );
	    break;
	case Types.VARCHAR :
	    date = Date.valueOf( (String)obj );
	    break;
	case Types.DATE :	// Convert to string
	    date = (Date)obj;
	    break;
	case Types.TIMESTAMP :
	    date = new Date( ((Timestamp)obj).getTime() );
	    break;
	}

	if ( date != null )
        {
	    String str = date.toString();
	    ParamSet.validateDateTimeStr( str );
	    return str;
        }
	break;

    case Types.TIME :
	Time	time = null;

	switch( objType )
	{
	case Types.CHAR :
	    time = Time.valueOf( String.copyValueOf( (char[])obj ) );
	    break;
	case Types.VARCHAR :
	    time = Time.valueOf( (String)obj );
	    break;
	case Types.TIME :	// Convert to string
	    time = (Time)obj;
	    break;
	case Types.TIMESTAMP :
	    time = new Time( ((Timestamp)obj).getTime() );
	    break;
	}

	if ( time != null )
        {
	    String str ;
            synchronized(dateFormatter)
            {
                str = dateFormatter.format( (java.util.Date)time );
            }
	    ParamSet.validateDateTimeStr( str );
	    return str;
        }
	break;

    case Types.TIMESTAMP :
	Timestamp   ts = null;

	switch( objType )
	{
	case Types.CHAR :
	    ts = Timestamp.valueOf( String.copyValueOf( (char[])obj ) );
	    break;
	case Types.VARCHAR :
	    ts = Timestamp.valueOf( (String)obj );
	    break;
	case Types.DATE :
	    ts = new Timestamp( ((Date)obj).getTime() );
	    break;
	case Types.TIMESTAMP :	// Convert to string
	    ts = (Timestamp)obj;
	    break;
	}

	if ( ts != null )
        {
            String str ;
            synchronized(dateFormatter)
            {
                str = dateFormatter.format( (java.util.Date)ts );
            }
	    ParamSet.validateDateTimeStr( str );
	    return str;
        }
	break;
    }

    throw new Exception( "Invalid coercion: " + objType + " to " + sqlType );
} // coerce


/*
** Name: str2bin
**
** Description:
**	Converts a hex string into a byte array.
**
** Input:
**	str	Hex string.
**
** Output:
**	None.
**
** Returns:
**	byte[]	Byte array.
**
** History:
**	10-May-01 (gordy)
**	    Created.
*/

private byte[]
str2bin( String str )
{
    byte    ba[] = new byte[ str.length() / 2 ];
    int	    offset = 0, i = 0;

    for( ; (offset + 1) < str.length(); offset += 2 )
	ba[ i++ ] = Byte.parseByte( str.substring(offset, offset + 2), 16 );
    return( ba );
}


/*
** Name: Param
**
** Description:
**	Parameter description and value.
**
**  Public data
**
**	type	    SQL type.
**	value	    Parameter value.
**	flags	    Descriptor flags.
**	name	    Parameter name.
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
**	11-Jan-01 (gordy)
**	    This class is Cloneable since parameter object
**	    values are immutable.
*/

private static class
Param
    implements Cloneable
{

    public  int		type = Types.NULL;
    public  Object	value = null;
    public  byte	flags = 0;
    public  String	name = null;


public Object
clone()
{
    Object cpy = null;

    try { cpy = super.clone(); }
    catch( CloneNotSupportedException ignore ) {}   // Should not happen!
    
    return( cpy );
}


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
*/

public void
clear()
{
    type = Types.NULL;
    value = null;
    flags = 0;
    name = null;
    return;
} // clear

} // class Param

/** Name: validateDateTimeStr
**
** Description:
**     Validates Date/Timestamp string.
**
** Input:
**     None.
**
** Output:
**     None.
**
** Returns:
**    void 
**
** History:
**     11-Feb-04 (rajus01) Bug #111769 Startrak #EDJDBC82
**         Created.
**     08-Aug-04 (rajus01) Bug #112774; Issue #13127367  
**	   setDate() with a valid date value passed in failed with error.
**	   
*/
public static void
validateDateTimeStr( String str )
throws SQLException
{
    boolean  error = false;
    String tmStr = null;
 
    if( str != null || str.length() > 0 )
    {
        if( str.indexOf(' ') != -1 )
            tmStr = str.substring(str.indexOf(' ') + 1); 

        if( tmStr == null )
        {
             // This is a date string
             if( str.lastIndexOf('-') != 7 || str.length() > 10 )
               error = true;
        }
        else 
        {
             // This is a Timestamp string of the following format
             // A valid Timestamp object is in format yyyy-mm-dd hh:mm:ss.fffffffff 
             if( str.indexOf(' ') != 10 || str.lastIndexOf(':') != 16 || str.length() > 29 )
                 error = true;
        }
    }

    if( error )
       throw EdbcEx.get( E_JD0006_CONVERSION_ERR );

    return;
}
} // class ParamSet
