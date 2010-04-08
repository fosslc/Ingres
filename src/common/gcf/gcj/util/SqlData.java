/*
** Copyright (c) 2009 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.util;

/*
** Name: SqlData.java
**
** Description:
**	Defines base class which provides support for SQL data values.
**
**  Classes:
**
**	SqlData		Base class for NULL data.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Added support for parameter types/values in addition to 
**	    existing support for columns.
**	15-Nov-06 (gordy)
**	    Support LOB Locators via Blob/Clob.
**      17-Nov-08 (rajus01) SIR 121238
**          Replaced SqlEx references with SQLException or SqlExFactory
**          depending upon the usage of it. SqlEx becomes obsolete to support
**          JDBC 4.0 SQLException hierarchy.
**      8-Apr-09 (rajus01) SIR 121238
**          Added getNlob()method for NCLOB support.
**      
*/

import	java.math.BigDecimal;
import	java.io.InputStream;
import	java.io.ByteArrayInputStream;
import	java.io.InputStreamReader;
import	java.io.Reader;
import	java.io.CharArrayReader;
import	java.io.StringReader;
import	java.io.UnsupportedEncodingException;
import	java.util.TimeZone;
import	java.sql.Blob;
import	java.sql.Clob;
import	java.sql.NClob;
import	java.sql.Date;
import	java.sql.Time;
import	java.sql.Timestamp;
import	java.sql.Types;
import	java.sql.SQLException;


/*
** Name: SqlData
**
** Description:
**	Utility base class which provides NULL data capabilities
**	and defines basic data access interface.
**
**	This class, in and of itself, only represents NULL typeless
**	data values and cannot be directly instantiated.  Sub-classes
**	provide support for specific datatype values.
**
**	This class defines routines for handling data truncation but
**	the routines are only implemented for the non-truncation case.
**	A separate sub-class should implement truncation support and
**	act as base class for support of truncatable datatypes.
**
**	This class implements a set of setXXX() and getXXX() methods 
**	which always throw conversions exceptions.  Sub-classes over-
**	ride those methods for which conversion of their specific 
**	datatype is supported.  This class provides implementations
**	of some methods, as described below, which may be utilized
**	or overridden by sub-classes.
**
**	Three getXXX() methods have implementations which accept a
**	length limit for the value returned.  These implementations
**	all call the standard versions of these methods (no limits).
**	Sub-classes which override the standard getXXX() methods may
**	need to override the length limited versions as well.
**
**	The getBigDecimal() method which takes a scale parameter is
**	implemented to apply the scale using setScale() to the value
**	returned by getBigDecimal() with no scale parameter.  Sub-
**	classes may override this method if a more efficient way
**	of applying the scale is available.
**
**	The setBigDecimal() method which takes a scale parameter is
**	implemented to simply call setBigDecimal() with no scale.
**	Note that in this context, the scale is applicable to the
**	final parameter type, not the input value.  If scale is 
**	applicable to a sub-class, it should override both methods 
**	(and should also override setScale(), see below).
**
**	The setObject() method implemented by this class attempts to
**	handle the scale parameter by passing it to setBigDecimal(),
**	as described above.  If the scale could not be used (some
**	other setXXX() method was called), the setScale() method is 
**	called with the scale value.  The default implementation of 
**	setScale() is a no-op.  If scale is applicable to the type
**	supported by a sub-class (including those which implement 
**	the setBigDecimal() method with scale), setScale() should
**	be overridden.
**
**	The setObject() methods implemented by this class are fully 
**	contained and translate to the other setXXX() methods based
**	on the object class.  Sub-classes should not need to over-
**	ride these implementations.
**
**  Public Methods:
**
**	getSqlType		Returns SQL type associated with Java object.
**	getBinary		Convert byte array into binary stream.
**	getAscii		Convert String into ASCII stream.
**	getUnicode		Convert String into Unicode stream.
**	getCharacter		Convert String into character stream.
**	cnvtAscii		Convert ASCII stream to character stream.
**	cnvtUnicode		Convert Unicode stream to character stream.
**
**	toString		String representation of data value.
**	isNull			Data value is NULL?
**	setNull			Set data value NULL.
**	isTruncated		Data value is truncated?
**	getDataSize		Expected size of data value.
**	getTruncSize		Actual size of truncated data value.
**
**	setBoolean		Data value accessor assignment methods
**	setByte
**	setShort
**	setInt
**	setLong
**	setFloat
**	setDouble
**	setBigDecimal
**	setString
**	setBytes
**	setDate
**	setTime
**	setTimestamp
**	setBinaryStream
**	setAsciiStream
**	setUnicodeStream
**	setCharacterStream
**	setBlob
**	setClob
**	setObject
**
**	getBoolean		Data value accessor retrieval methods
**	getByte
**	getShort
**	getInt
**	getLong
**	getFloat
**	getDouble
**	getBigDecimal
**	getString
**	getBytes
**	getDate
**	getTime
**	getTimestamp
**	getBinaryStream
**	getAsciiStream
**	getUnicodeStream
**	getCharacterStream
**	setBlob
**	setClob
**	getObject
**
**  Protected Methods:
**
**	setNotNull		Set data value not NULL.
**	setScale		Set DECIMAL scale.
**
**  Private Data:
**
**	is_null			Data value is NULL?.
**
**  Private Methods:
**
**	setObject		Default implementation.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
**	 1-Dec-03 (gordy)
**	    Added default setXXX() methods and helper methods getSqlType(),
**	    getBinary(), getAscii(), getUnicode(), getCharacter(), cnvtAscii(),
**	    and cnvtUnicode() to support parameter data values.
**	15-Nov-06 (gordy)
**	    Added getBlob(), getClob(), setBlob, setClob() to support
**	    LOB Locators.
*/

public class
SqlData
    implements GcfErr
{

    private boolean		is_null = true;


/*
** Name: getSqlType
**
** Description:
**	Returns the SQL type associated with a Java object 
**	as indicated by the following table:
**
**	Java Type		SQL Type
**	------------		--------
**	null			NULL
**	java.lang.Boolean	BOOLEAN
**	java.lang.Byte		TINYINT
**	java.lang.Short		SMALLINT
**	java.lang.Integer	INTEGER
**	java.lang.Long		BIGINT
**	java.lang.Float		REAL
**	java.lang.Double	DOUBLE
**	java.math.BigDecimal	DECIMAL
**	byte[]			VARBINARY
**	java.io.InputStream	LONGVARBINARY
**	java.sql.Blob		BLOB
**	char[]			CHAR
**	java.lang.String	VARCHAR
**	java.io.Reader		LONGVARCHAR
**	java.sql.Clob		CLOB
**	java.sql.Date		DATE
**	java.sql.Time		TIME
**	java.sql.Timestamp	TIMESTAMP
**
** Input:
**	obj	Parameter value.
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
**	19-Feb-03 (gordy)
**	    Replaced BIT with BOOLEAN.  Give priority to strings since
**	    alternate storage class results in calls to this routine.
**	 1-Dec-03 (gordy)
**	    Extracted from parameter set class.
**	15-Nov-06 (gordy)
**	    Support LOB Locators via Blob/Clob.
**	29-Apr-09 (rajus01) SD issue 135502; Bug # 121965
**	    Due to the behaviour of the byte type, the driver should support byte arrays using the
**	    varbyte type. Hence changing SQL type of the byte[] from BINARY to VARBINARY.
*/

public static int
getSqlType( Object obj )
    throws SQLException
{
    if ( obj == null )			return( Types.NULL );
    if ( obj instanceof String )	return( Types.VARCHAR );
    if ( obj instanceof Boolean )	return( Types.BOOLEAN );
    if ( obj instanceof Byte )		return( Types.TINYINT );
    if ( obj instanceof Short )		return( Types.SMALLINT );
    if ( obj instanceof Integer )	return( Types.INTEGER );
    if ( obj instanceof Long )		return( Types.BIGINT );
    if ( obj instanceof Float )		return( Types.REAL );
    if ( obj instanceof Double )	return( Types.DOUBLE );
    if ( obj instanceof BigDecimal )	return( Types.DECIMAL );
    if ( obj instanceof Reader )	return( Types.LONGVARCHAR );
    if ( obj instanceof InputStream )	return( Types.LONGVARBINARY );
    if ( obj instanceof Blob )		return( Types.BLOB );
    if ( obj instanceof Clob )		return( Types.CLOB );
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

    throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
} // getSqlType


/*
** Name: getBinary
**
** Description:
**	Helper method to convert byte array to binary stream.
**
** Input:
**	array		Byte array.
**	offset		Starting byte of stream.
**	length		Number of bytes in stream.
**
** Output:
**	None.
**
** Returns:
**	InputStream    Binary stream.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public static InputStream
getBinary( byte array[], int offset, int length ) 
    throws SQLException
{ 
    return( new ByteArrayInputStream( array, offset, length ) );
} // getBinary


/*
** Name: getAscii
**
** Description:
**	Helper method to convert String into ASCII stream.
**
** Input:
**	str		String to be converted.
**
** Output:
**	None.
**
** Returns:
**	InputStream    ASCII stream.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public static InputStream
getAscii( String str ) 
    throws SQLException
{ 
    byte	bytes[];
    
    try { bytes = str.getBytes( "US-ASCII" ); }
    catch( UnsupportedEncodingException ex )		// Should not happen!
    { throw SqlExFactory.get( ERR_GC401E_CHAR_ENCODE ); }
    
    return( new ByteArrayInputStream( bytes ) );
} // getAscii


/*
** Name: getUnicode
**
** Description:
**	Helper method to convert String into Unicode stream.
**
** Input:
**	str		String to be converted.
**
** Output:
**	None.
**
** Returns:
**	InputStream	Unicode stream.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public static InputStream
getUnicode( String str ) 
    throws SQLException
{ 
    byte	bytes[];
    
    try { bytes = str.getBytes( "UTF-8" ); }
    catch( UnsupportedEncodingException ex )		// Should not happen!
    { throw SqlExFactory.get( ERR_GC401E_CHAR_ENCODE ); }
    
    return( new ByteArrayInputStream( bytes ) );
} // getUnicode


/*
** Name: getCharacter
**
** Description:
**	Helper method to convert character array into stream.
**
** Input:
**	array	Character array to be converted.
**
** Output:
**	None.
**
** Returns:
**	Reader	Character stream.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public static Reader
getCharacter( char array[], int offset, int length ) 
    throws SQLException
{ 
    return( new CharArrayReader( array, offset, length ) );
} // getCharacter


/*
** Name: getCharacter
**
** Description:
**	Helper method to convert String into character stream.
**
** Input:
**	str	String to be converted.
**
** Output:
**	None.
**
** Returns:
**	Reader	Character stream.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public static Reader
getCharacter( String str ) 
    throws SQLException
{ 
    return( new StringReader( str ) );
} // getCharacter


/*
** Name: cnvtAscii
**
** Description:
**	Helper method to convert ASCII stream into 
**	character stream.
**
** Input:
**	stream	ASCII stream to be converted.
**
** Output:
**	None.
**
** Returns:
**	Reader	Character stream.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public static Reader
cnvtAscii( InputStream stream )
    throws SQLException
{
    try { return( new InputStreamReader( stream, "US-ASCII" ) ); }
    catch( UnsupportedEncodingException ex )
    { throw SqlExFactory.get( ERR_GC401E_CHAR_ENCODE ); }	// Should not happen!
} // cnvtAscii


/*
** Name: cnvtUnicode
**
** Description:
**	Helper method to convert Unicode stream into 
**	character stream.
**
** Input:
**	stream	Unicode stream to be converted.
**
** Output:
**	None.
**
** Returns:
**	Reader	Character stream.
**
** History:
**	 1-Dec-03 (gordy)
**	    Created.
*/

public static Reader
cnvtUnicode( InputStream stream )
    throws SQLException
{
    try { return( new InputStreamReader( stream, "UTF-8" ) ); }
    catch( UnsupportedEncodingException ex )
    { throw SqlExFactory.get( ERR_GC401E_CHAR_ENCODE ); }	// Should not happen!
} // cnvtUnicode


/*
** Name: SqlData
**
** Description:
**	Constructor for sub-classes to define initial NULL
**	state of the data value.
**
** Input:
**	is_null		True if data value is initially NULL.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
*/

protected
SqlData( boolean is_null )
{
    this.is_null = is_null;
} // SqlData


/*
** Name: toString
**
** Description:
**	Returns a string representing the current object.
**
**	Overrides super-class method to attempt conversion
**	of SQL value to string using getString() method.
**	Since getString() may not be implemented by a sub-
**	class, an exception from getString() defaults to
**	using the super-class method.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	String representation of this SQL data value.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
*/

public String
toString()
{
    String str;
    
    try { str = getString(); }
    catch( SQLException ignore ) 
    { str = super.toString(); }
    
    return( str );
} // toString


/*
** Name: isNull
**
** Description:
**	Returns the NULL state of the data value.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean		True if data value is NULL.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
*/

public boolean
isNull()
{
    return( is_null );
} // isNull


/*
** Name: setNull
**
** Description:
**	Set the NULL state of the data value to NULL.
**
**	Sub-classes may need to override this method if the
**	stored data value is dependent on the NULL state.
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
**	 5-Sep-03 (gordy)
**	    Created.
*/

public void
setNull()
{
    is_null = true;
    return;
} // setNull


/*
** Name: setNotNull
**
** Description:
**	Set the NULL state of the data value to not NULL.
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
**	 5-Sep-03 (gordy)
**	    Created.
*/

protected void
setNotNull()
{
    is_null = false;
    return;
} // setNotNull


/*
** Name: isTruncated
**
** Description:
**	Returns an indication that the data value was truncated.
**
**	This implementation always returns False.  Sub-classes should 
**	override this method to support truncatable datatypes.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean		True if truncated.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
*/

public boolean
isTruncated()
{
    return( false );
} // isTruncated


/*
** Name: getDataSize
**
** Description:
**	Returns the expected size of an untruncated data value.
**	For datatypes whose size is unknown or varies, -1 is returned.
**	The returned value is only valid for truncated data values
**	(isTruncated() returns True).
**
**	This implementation always returns -1.  Sub-classes should 
**	override this method to support truncatable datatypes.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int    Expected size of data value.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
*/

public int
getDataSize()
{
    return( -1 );
} // getDataSize


/*
** Name: getTruncSize
**
** Description:
**	Returns the actual size of a truncated data value.  For 
**	datatypes whose size is unknown or varies, -1 is returned.
**	The returned value is only valid for truncated data values
**	(isTruncated() returns True).
**
**	This implementation always returns 0.  Sub-classes should 
**	override this method to support truncatable datatypes.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int    Truncated size of data value.
**
** History:
**	 5-Sep-03 (gordy)
**	    Created.
*/

public int
getTruncSize()
{
    return( 0 );
} // getTruncSize


/*
** Name: setScale
**
** Description:
**	Set decimal scale of SQL data value.
**
**	Default implementation is no-op.  Sub-classes which
**	have scale as an attribute of their SQL type should
**	override this method.  
**
** Input:
**	scale	Number of decimal places.
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

protected void
setScale( int scale ) throws SQLException
{ 
    return; 
} // setScale


/*
** Name: setObject
**
** Description:
**	Assign a Java Object as the data value.
**	The data value will be NULL if the input 
**	value is null, otherwise non-NULL.
**
**	The scale input is only referenced if
**	permitted by the use_scale input, and
**	then only for applicable SQL types.
**
**	Note: the object class coercion is dependent
**	on the class/SQL type association performed
**	by the getSqlType() method.
**
** Input:
**	value		Java Object (may be null).
**	use_scale	True if scale is valid.
**	scale		Scale.
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
**	15-Nov-06 (gordy)
**	    Support LOB Locators via Blob/Clob.
**	29-Apr-09 (rajus01) SD issue 135502, Bug # 121965.
**	    Eventhough BINARY case is not needed I just left it there and added VARBINARY.
*/

private void 
setObject( Object value, boolean use_scale, int scale ) 
    throws SQLException
{
    switch( getSqlType( value ) )
    {
    case Types.NULL :		setNull();				break;
    case Types.BOOLEAN : setBoolean( ((Boolean)value).booleanValue() );	break;
    case Types.TINYINT :	setByte( ((Byte)value).byteValue() );	break;
    case Types.SMALLINT :	setShort(((Short)value).shortValue());	break;
    case Types.INTEGER :	setInt( ((Integer)value).intValue() );	break;
    case Types.BIGINT :		setLong( ((Long)value).longValue() );	break;
    case Types.REAL :		setFloat(((Float)value).floatValue());	break;
    case Types.DOUBLE :		setDouble(((Double)value).doubleValue()); break;
    case Types.BINARY :		
    case Types.VARBINARY :	setBytes( (byte[])value );		break;
    case Types.LONGVARBINARY :	setBinaryStream( (InputStream)value );	break;
    case Types.BLOB :		setBlob( (Blob)value );			break;
    case Types.CHAR :		setString( new String((char[])value) );	break;
    case Types.VARCHAR :	setString( (String)value );		break;
    case Types.LONGVARCHAR :	setCharacterStream( (Reader)value );	break;
    case Types.CLOB :		setClob( (Clob)value );			break;
    case Types.DATE :		setDate( (Date)value, null );		break;
    case Types.TIME :		setTime( (Time)value, null );		break;
    case Types.TIMESTAMP :	setTimestamp( (Timestamp)value, null );	break;
	
    case Types.DECIMAL :
	if ( ! use_scale )
	    setBigDecimal( (BigDecimal)value );	
	else
	{
	    setBigDecimal( (BigDecimal)value, scale );
	    use_scale = false;	// Scale used, don't reference further.
	}
	break;
	
    default :
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );	// Shouldn't happen
    }
    
    /*
    ** If scale could not be applied above, it may still
    ** be applied if a sub-class implements setScale().
    */
    if ( use_scale )  setScale( scale );
    
    return;
} // setObject


/*
** The following routines represent the set of valid accessor
** assignment methods for the data values supported by SQL.  
** For this implementation these methods throw data conversion 
** errors.  Sub-classes should override those methods for which 
** conversion of their datatype is supported.
*/

public void 
setBoolean( boolean value ) throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public void 
setByte( byte value ) throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public void 
setShort( short value ) throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public void 
setInt( int value ) throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public void 
setLong( long value ) throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public void 
setFloat( float value ) throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public void 
setDouble( double value ) throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public void 
setBigDecimal( BigDecimal value ) throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public void 
setString( String value ) throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public void 
setBytes( byte value[] ) throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public void 
setDate( Date value, TimeZone tz ) throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public void 
setTime( Time value, TimeZone tz ) throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public void 
setTimestamp( Timestamp value, TimeZone tz ) throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public void 
setBinaryStream( InputStream value ) throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public void 
setAsciiStream( InputStream value ) throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public void 
setUnicodeStream( InputStream value ) throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public void 
setCharacterStream( Reader value ) throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public void 
setBlob( Blob value ) throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public void 
setClob( Clob value ) throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }


/*
** The following routines represent the set of valid accessor
** retrieval methods for the data values supported by SQL.  
** For this implementation these methods throw data conversion 
** errors.  Sub-classes should override those methods for which 
** conversion of their datatype is supported.
*/

public boolean 
getBoolean() throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public byte 
getByte() throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public short 
getShort() throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public int 
getInt() throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public long 
getLong() throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public float 
getFloat() throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public double 
getDouble() throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public BigDecimal 
getBigDecimal() throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public String 
getString() throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public byte[] 
getBytes() throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public Date 
getDate( TimeZone tz ) throws SQLException
{  throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public Time 
getTime( TimeZone tz ) throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public Timestamp 
getTimestamp( TimeZone tz ) throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public InputStream 
getBinaryStream() throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public InputStream 
getAsciiStream() throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public InputStream 
getUnicodeStream() throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public Reader 
getCharacterStream() throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public Blob
getBlob() throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public Clob
getClob() throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public Object 
getObject() throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

public NClob
getNlob() throws SQLException
{ throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); }

/*
** The following methods are just covers for other
** stub methods implemented above.  A sub-class may
** implement these methods if parameter depedencies
** exist.
*/

public void 
setBigDecimal( BigDecimal value, int scale ) throws SQLException
{ setBigDecimal( value ); }

public void 
setObject( Object value ) throws SQLException
{ setObject( value, false, 0 ); }

public void 
setObject( Object value, int scale ) throws SQLException
{ setObject( value, true, scale ); }

public BigDecimal 
getBigDecimal( int scale ) throws SQLException
{ BigDecimal bd = getBigDecimal();
  return((bd == null) ? null : bd.setScale(scale, BigDecimal.ROUND_HALF_UP)); }

public String
getString( int limit ) throws SQLException
{ return( getString() ); }

public byte[]
getBytes( int limit ) throws SQLException
{ return( getBytes() ); }

public Object
getObject( int limit ) throws SQLException
{ return( getObject() ); }


} // class SqlData
