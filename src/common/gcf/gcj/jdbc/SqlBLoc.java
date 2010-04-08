/*
** Copyright (c) 2009 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: SqlBLoc.java
**
** Description:
**	Defines class which represents a Binary LOB Locator value.
**
**  Classes:
**
**	SqlBLoc		A Binary LOB Locator value.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	30-Jan-07 (gordy)
**	    Backward compatibility for LOB Locators.
**	 4-May-07 (gordy)
**	    Set class access for reflection.
**      05-Jan-09 (rajus01) SIR 121238
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to
**            support JDBC 4.0 SQLException hierarchy.
*/

import	java.io.InputStream;
import	java.io.Reader;
import	java.sql.Blob;
import	java.sql.SQLException;
import	com.ingres.gcf.util.SqlLoc;
import	com.ingres.gcf.util.SqlVarByte;
import	com.ingres.gcf.util.SqlLongByte;
import	com.ingres.gcf.util.SqlStream;
import	com.ingres.gcf.util.SqlExFactory;


/*
** Name: SqlBLoc
**
** Description:
**	Class which represents a Binary LOB Locator value.
**
**	Extends the utility SqlLoc class to add the ability to
**	convert the locator into supported JDBC data types.
**	Supports conversion to Blob and Object.  For backward
**	compatibility, additional accessor retrieval methods
**	are supported to match functionality of SqlLongByte.
**
**	The data value accessor methods do not check for the NULL 
**	condition.  The super-class isNull() method should first 
**	be checked prior to calling any of the accessor methods.
**
**  Public Methods:
**
**	set			Assign a new data value.
**
**	setBlob			Data value accessor assignment methods.
**
**	getBlob			Data value accessor retrieval methods.
**	getString
**	getBytes
**	getBinaryStream	
**	getAsciiStream
**	getUnicodeStream
**	getCharacterStream
**	getObject
**
**  Private Data:
**
**	conn			DBMS connection.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	30-Jan-07 (gordy)
**	    Added stream accessor retrieval methods getString(), getBytes(),
**	    getBinaryStream(), getAsciiStream(), getUnicodeStream(),
**	    getCharacterStream().
**	 4-May-07 (gordy)
**	    Class is not exposed outside package, restrict access.
*/

class	// package access
SqlBLoc
    extends SqlLoc
{


    private DrvConn		conn = null;


/*
** Name: SqlBLoc
**
** Description:
**	Class constructor.  Data value is initially NULL.
**
** Input:
**	conn	DBMS connection.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public
SqlBLoc( DrvConn conn )
    throws SQLException
{
    super();
    this.conn = conn;
} // SqlBLoc


/*
** Name: set
**
** Description:
**	Assign a new data value as a copy of an existing 
**	SQL data object.  If the input is NULL, a NULL 
**	data value results.
**
** Input:
**	data	The SQL data to be copied.
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
set( SqlBLoc data )
{
    if ( data == null  ||  data.isNull() )
	setNull();
    else
    {
	conn = data.conn;
	set( (SqlLoc)data );
    }
    return;
} // set


/*
** Name: setBlob
**
** Description:
**	Assign a Blob object as the non-NULL data value.
**
**	While the generic interface allows any Blob object
**	to be assigned, this method only supports DrvBlob
**	objects.
**	
** Input:
**	value	Blob object.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public void
setBlob( Blob value ) 
    throws SQLException
{ 
    if ( value == null )
	setNull();
    else  if ( value instanceof DrvBlob )
	set( ((DrvBlob)value).getLocator() );
    else
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR ); 

    return;
} // setBlob


/*
** Name: getBlob
**
** Description:
**	Returns a Blob wrapper for the LOB Locator
**	which can access the LOB data in the database.
**	The Blob wrapper can also manage the LOB as
**	buffered LOB data.
**	
**      Note: the value returned when the data value is
**      NULL is not well defined.
**
** Input:
**	None.
**	
** Output:
**	None.
**
** Returns:
**	Blob	Blob wrapper for LOB Locator.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	26-Feb-07 (gordy)
**	    Added capability to configure cache segment size.
*/

public Blob
getBlob()
    throws SQLException
{
    /*
    ** The LOB Locator is first wrapped in a class
    ** which is able to access the LOB data in the
    ** database.  The locator Blob is then wrapped 
    ** in a class which can convert and manage the
    ** LOB as updatable buffered data.
    */
    return( new JdbcBlob( new DrvBlob( conn, value ), 
			  conn.cnf_lob_segSize, conn.trace ) );
} // getBlob


/*
** Name: getString
**
** Description:
**	Return the current data value as a String value.
**
**	Note: the value returned when the data value is
**	NULL is not well defined.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	Current value converted to String.
**
** History:
**	30-Jan-07 (gordy)
**	    Created.
*/

public String
getString()
    throws SQLException
{
    byte ba[] = SqlLongByte.strm2array( getBinaryStream() );
    return( SqlVarByte.bin2str( ba, 0, ba.length ) );
} // getString


/*
** Name: getString
**
** Description:
**	Return the current data value as a String value
**	with a maximum size limit.
**
**	Note: the value returned when the data value is
**	NULL is not well defined.
**
** Input:
**	limit	Maximum length of result.
**
** Output:
**	None.
**
** Returns:
**	String	Current value converted to String.
**
** History:
**	30-Jan-07 (gordy)
**	    Created.
*/

public String
getString( int limit )
    throws SQLException
{
    byte ba[] = SqlLongByte.strm2array( getBinaryStream(), limit );
    return( SqlVarByte.bin2str( ba, 0, ba.length ) );
} // getString


/*
** Name: getBytes
**
** Description:
**	Return the current data value as a byte array.
**
**	Note: the value returned when the data value is
**	NULL is not well defined.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	Current value converted to byte array.
**
** History:
**	30-Jan-07 (gordy)
**	    Created.
*/

public byte[]
getBytes()
    throws SQLException
{
    return( SqlLongByte.strm2array( getBinaryStream() ) );
} // getBytes


/*
** Name: getBytes
**
** Description:
**	Return the current data value as a byte array
**	with a maximum size limit.
**
**	Note: the value returned when the data value is
**	NULL is not well defined.
**
** Input:
**	limit	Maximum length of result.
**
** Output:
**	None.
**
** Returns:
**	String	Current value converted to byte array.
**
** History:
**	30-Jan-07 (gordy)
**	    Created.
*/

public byte[]
getBytes( int limit )
    throws SQLException
{
    return( SqlLongByte.strm2array( getBinaryStream(), limit ) );
} // getBytes


/*
** Name: getBinaryStream
**
** Description:
**	Return the current data value as a binary stream.
**
**	Note: the value returned when the data value is
**	NULL is not well defined.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	InputStream	Current value converted to binary stream.
**
** History:
**	30-Jan-07 (gordy)
**	    Created.
**	02-Feb-09 (rajus01) SIR 121238
**	    Code cleanup: Removed try...catch() block as SqlEx is gone.
*/

public InputStream
getBinaryStream()
    throws SQLException
{
    /*
    ** Generate Blob wrapper which provides stream access.
    */
    InputStream	is;
    Blob	blob = new DrvBlob( conn, value );

    is = blob.getBinaryStream();

    return( is );
} // getBinaryStream


/*
** Name: getAsciiStream
**
** Description:
**	Return the current data value as an ASCII stream.
**
**	Note: the value returned when the data value is
**	NULL is not well defined.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	InputStream	Current value converted to ASCII stream.
**
** History:
**	30-Jan-07 (gordy)
**	    Created.
*/

public InputStream
getAsciiStream()
    throws SQLException
{
    return( SqlStream.getAsciiIS( getCharacterStream() ) );
} // getAsciiStream


/*
** Name: getUnicodeStream
**
** Description:
**	Return the current data value as a Unicode stream.
**
**	Note: the value returned when the data value is
**	NULL is not well defined.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	InputStream	Current value converted to Unicode stream.
**
** History:
**	30-Jan-07 (gordy)
**	    Created.
*/

public InputStream
getUnicodeStream()
    throws SQLException
{
    return( SqlStream.getUnicodeIS( getCharacterStream() ) );
} // getUnicodeStream


/*
** Name: getCharacterStream
**
** Description:
**	Return the current data value as a character stream.
**
**	Note: the value returned when the data value is
**	NULL is not well defined.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Reader	Current value converted to character stream.
**
** History:
**	30-Jan-07 (gordy)
**	    Created.
*/

public Reader
getCharacterStream()
    throws SQLException
{
    return( SqlLongByte.byteIS2hexRdr( getBinaryStream() ) );
} // getCharacterStream


/*
** Name: getObject
**
** Description:
**      Return the current data value as an Binary object.
**
**      Note: the value returned when the data value is
**      NULL is not well defined.
**
** Input:
**      None.
**
** Output:
**      None.
**
** Returns:
**      Object  Current value converted to Binary.
**
** History:
**      15-Nov-06 (gordy)
**          Created.
*/

public Object
getObject()
    throws SQLException
{
    return( getBlob() );
} // getObject


} // class SqlBLoc
