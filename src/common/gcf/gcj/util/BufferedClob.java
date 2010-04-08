/*
** Copyright (c) 2009 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.util;

/*
** Name: BufferedClob.java
**
** Description:
**	Defines class which implements the JDBC Clob interface
**	as a wrapper for a cached CLOB value.
**
**  Classes:
**
**	BufferedClob		Clob wrapper for CharBuffer
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
**	26-Feb-07 (gordy)
**	    Support buffered LOBs.
**	 8-Jun-09 (gordy)
**	    Implemented methods for JDBC 4.0.
*/

import	java.io.InputStream;
import	java.io.OutputStream;
import	java.io.Reader;
import	java.io.Writer;
import	java.sql.Clob;
import	java.sql.SQLException;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.util.GcfErr;


/*
** Name: BufferedClob
**
** Description:
**	Class which implements the JDBC Clob interface as
**	a wrapper for a cached CLOB value.
**
**	The CLOB data is stored in a CharBuffer.
**
**  Interface Methods:
**
**	free			Release resources.
**	getAsciiStream		Retrieve stream which accesses LOB data.
**	getCharacterStream	Retrieve stream which accesses LOB data.
**	getSubString		Read characters from LOB data.
**	length			Get length of LOB data.
**	position		Search LOB data.
**	setAsciiStream		Write LOB data from stream.
**	setCharacterStream	Write LOB data from stream.
**	setString		Write characters into LOB data.
**	truncate		Set length of LOB data.
**
**  Private Data:
**
**	buff			Underlying cached CLOB.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
**	26-Feb-07 (gordy)
**	    Added constructor to wrap existing char buffer.
**      17-Nov-08 (rajus01) SIR 121238
**          - Added new JDBC 4.0 methods to avoid compilation errors with
**            JDK 1.6. The new methods return E_GC4019 error for now.
**          - Replaced SqlEx references with SqlExFactory as SqlEx becomes
**            obsolete to support JDBC 4.0 SQLException hierarchy.
**	 8-Jun-09 (gordy)
**	    Added checkValid() to check validity of LOB object.
*/

public class
BufferedClob
    implements Clob, GcfErr
{

    private CharBuffer		buff = null;


/*
** Name: BufferedClob
**
** Description:
**	Class constructor.  Constructs an empty Clob
**	with default segment size.
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
**	 2-Feb-07 (gordy)
**	    Created.
*/

public
BufferedClob()
{
    buff = new CharBuffer();
} // BufferedClob


/*
** Name: BufferedClob
**
** Description:
**	Class constructor.  Constructs an empty Clob
**	with requested segment size.
**
** Input:
**	size	Block size.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public
BufferedClob( int size )
{
    buff = new CharBuffer( size );
} // BufferedClob


/*
** Name: BufferedClob
**
** Description:
**	Class constructor.  Constructs an empty Clob
**	with default segment size and fills with data
**	read from a character stream.  The character 
**	stream is closed.
**
** Input:
**	stream	Character stream.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public
BufferedClob( Reader stream )
{
    buff = new CharBuffer( stream );
} // BufferedClob


/*
** Name: BufferedClob
**
** Description:
**	Class constructor.  Constructs an empty Clob
**	with requested segment size and fills with data
**	read from a character stream.  The character 
**	stream is closed.
**
** Input:
**	size	Block size.
**	stream	Character stream.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
*/

public
BufferedClob( int size, Reader stream )
{
    buff = new CharBuffer( size, stream );
} // BufferedClob

    
/*
** Name: BufferedClob
**
** Description:
**	Class constructor.  Constructs a Clob as a
**	wrapper for an existing char buffer.
**
** Input:
**	buff	Character buffer.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	26-Feb-07 (gordy)
**	    Created.
*/

public
BufferedClob( CharBuffer buff )
{
    this.buff = buff;
} // BufferedClob


/*
** Name: free
**
** Description:
**    This method frees the Clob object and releases the resources that it
**    holds. The object is invalid once the free method is called.
**
**    After free has been called, any attempt to invoke a method other than
**    free will result in a SQLException being thrown. If free is called
**    multiple times, the subsequent calls to free are treated as a no-op.
**
** Input:
**      None.
**
** Output:
**      None.
**
** Returns:
**      None.
**
** History:
**      17-Nov-08 (rajus01) SIR 121238
**	    Created.
**	 8-Jun-09 (gordy)
**	    Implemented.
*/

public void 
free()
    throws SQLException
{
    if ( buff != null )
    {
	buff.free();
	buff = null;
    }

    return;
} // free


/*
** Name: length
**
** Description:
**	Returns the length of the Clob.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	long	Length of Clob.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
**	 8-Jun-09 (gordy)
**	    Validate LOB object.
*/

public long
length()
    throws SQLException
{
    checkValid();
    return( buff.length() );
} // length


/*
** Name: getSubString
**
** Description:
**      Extract portion of Clob.
**
** Input:
**      pos     Target position (1 based reference).
**      length  Number of characters.
**
** Output:
**      None.
**
** Returns:
**      String  Characters extracted.
**
** History:
**       2-Feb-07 (gordy)
**          Created.
**	 8-Jun-09 (gordy)
**	    Validate LOB object.
*/

public String
getSubString( long pos, int length )
    throws SQLException
{
    checkValid();

    /*
    ** Validate parameters.
    */
    if ( pos < 1  ||  length < 0 )
	throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );

    /*
    ** Adjust position to 0 base reference within valid
    ** range and limit length to amount of data available.
    */
    long limit = buff.length();
    pos = Math.min( pos - 1, limit );
    length = (int)Math.max( 0L, Math.min( limit - pos, (long)length ) );
    char data[] = new char[ length ];

    /*
    ** Due to the validation and adjustments done above,
    ** there should be no problem reading the data, but
    ** make sure the actual length matches requested.
    */
    if ( length > 0  &&  (int)buff.read( pos, data, 0, length ) != length )
	throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );

    return( new String( data ) );
} // getSubString


/*
** Name: getCharacterStream
**
** Description:
**	Returns a stream capable of reading the Clob data.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Reader	Stream which accesses Clob data.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
**	 8-Jun-09 (gordy)
**	    Validate LOB object.
*/

public Reader
getCharacterStream()
    throws SQLException
{
    checkValid();
    return( buff.getRdr() );
} // getCharacterStream


/*
** Name: getCharacterStream
**
** Description:
**	Returns a stream capable of reading the Clob data.
**
** Input:
**    pos	The offset to the first character of the partial value to be 
**		retrieved. The first character in the Clob is at position 1.
**    length	The length in characters of the partial value to be retrieved. 
**
** Output:
**	None.
**
** Returns:
**	Reader	Stream which accesses Clob data.
**
** History:
**	 17-Nov-08 (rajus01) SIR 121238
**	    Created.
**	 8-Jun-09 (gordy)
**	    Implemented.
*/

public Reader 
getCharacterStream( long pos, long length )
    throws SQLException
{
    checkValid();

    /*
    ** Validate parameters.
    */
    if ( pos < 1  ||  length < 0 )
	throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );

    return( buff.getRdr( pos - 1, length ) );
} // getCharacterStream


/*
** Name: getAsciiStream
**
** Description:
**      Returns a stream capable of reading and converting
**      the Clob data into ASCII.
**
** Input:
**      None.
**
** Output:
**      None.
**
** Returns:
**      InputStream     Stream which accesses Clob data.
**
** History:
**       2-Feb-07 (gordy)
**          Created.
**	 8-Jun-09 (gordy)
**	    Validate LOB object.
*/

public InputStream
getAsciiStream()
    throws SQLException
{
    checkValid();
    return( SqlStream.getAsciiIS( buff.getRdr() ) );
} // getAsciiStream


/*
** Name: position
**
** Description:
**	Search Clob for matching pattern.  If no match
**	is found, -1 is returned.
**
** Input:
**	pattern	Character pattern to be matched.
**	start	Starting position of search (1 based reference).
**
** Output:
**	None.
**
** Returns:
**	long	Position of match (1 based reference) or -1.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
**	 8-Jun-09 (gordy)
**	    Validate LOB object.
*/

public long
position( String pattern, long start )
    throws SQLException
{
    checkValid();

    if ( pattern == null  ||  start < 1 )
	throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );

    /*
    ** Adjust start and match position for 0 based reference.
    */
    start = buff.find( pattern, start - 1 );
    return( (start >= 0) ? start + 1 : -1 );
} // position


/*
** Name: position
**
** Description:
**	Search Clob for matching pattern.  If no match
**	is found, -1 is returned.
**
** Input:
**	pattern	Clob pattern to be matched.
**	start	Starting position of search (1 based reference).
**
** Output:
**	None.
**
** Returns:
**	long	Position of match (1 based reference) or -1.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
**	 8-Jun-09 (gordy)
**	    Validate LOB object.
*/

public long
position( Clob pattern, long start )
    throws SQLException
{
    checkValid();

    /*
    ** An extermely long pattern is not expected, but
    ** check to make sure the downcast won't lose bits.
    */
    long length;

    if ( pattern == null  ||  (length = pattern.length()) > Integer.MAX_VALUE )
	throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    return( position( pattern.getSubString( 1, (int)length ), start ) );
} // position


/*
** Name: setString
**
** Description:
**	Writes string into Clob at requested position.
**
** Input:
**	pos	Position in Clob to write string (1 based reference).
**	str	String to be written.
**
** Output:
**	None.
**
** Returns:
**	int	Number of characters written.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
**	 8-Jun-09 (gordy)
**	    Validate LOB object.
*/

public int
setString( long pos, String str )
    throws SQLException
{
    checkValid();
    if ( str == null )  throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    return( setString( pos, str, 0, str.length() ) );
} // setString


/*
** Name: setString
**
** Description:
**	Writes characters into Clob at requested position.
**
** Input:
**	pos	Position in Clob to write characters (1 based reference).
**	bytes	Characters to be written.
**	offset	Offset of characters to be written (0 based reference).
**	len	Number of characters to be written.
**
** Output:
**	None.
**
** Returns:
**	int	Number of charactrs written.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
**	 8-Jun-09 (gordy)
**	    Validate LOB object.
*/

public int
setString( long pos, String str, int offset, int len )
    throws SQLException
{
    checkValid();

    /*
    ** Validate parameters.
    */
    if ( str == null  ||  len < 0  ||
	 pos < 1  ||  pos > buff.length() + 1  ||
	 offset < 0  ||  offset >= str.length() )
	throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );

    /*
    ** Adjust target position to 0 based reference.
    */
    return( (int)buff.write( pos - 1, str, offset, len ) );
} // setString


/*
** Name: setCharacterStream
**
** Description:
**	Returns a stream capable of writing to the Clob at
**	the requested position.
**
** Input:
**	pos	Position to write stream (1 based reference).
**
** Output:
**	None.
**
** Returns:
**	Writer	Stream which writes to Clob.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
**	 8-Jun-09 (gordy)
**	    Validate LOB object.
*/

public Writer
setCharacterStream( long pos )
    throws SQLException
{
    checkValid();

    /*
    ** NULL is returned if target position is invalid.
    ** Adjust target position to 0 based reference.
    */
    Writer wtr;

    if ( (wtr = buff.getWtr( pos - 1 )) == null )
	throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );

    return( wtr );
} // setCharacterStream


/*
** Name: setAsciiStream
**
** Description:
**      Returns a stream capable of writing ASCII data
**      to the Clob at the requested position.
**
** Input:
**      pos             Position to write stream.
**
** Output:
**      None.
**
** Returns:
**      OutputStream    Stream which writes to Clob.
**
** History:
**       2-Feb-07 (gordy)
**          Created.
**	 8-Jun-09 (gordy)
**	    Validate LOB object.
*/

public OutputStream
setAsciiStream( long pos )
    throws SQLException
{
    checkValid();

    /*
    ** NULL is returned if target position is invalid.
    ** Adjust target position to 0 based reference.
    */
    Writer wtr;

    if ( (wtr = buff.getWtr( pos - 1 )) == null )
	throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );

    return( SqlStream.getAsciiOS( wtr ) );
} // setAsciiStream


/*
** Name: truncate
**
** Description:
**	Truncate Clob to requested length.
**
** Input:
**	len	Length at which Clob should be truncated.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 2-Feb-07 (gordy)
**	    Created.
**	 8-Jun-09 (gordy)
**	    Validate LOB object.
*/

public void
truncate( long len )
    throws SQLException
{
    checkValid();
    if ( len < 0 )  throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    buff.truncate( len );
    return;
} // truncate


/*
** Name: checkValid
**
** Description:
**	Checks validity of this CLOB object and throws an
**	exception if it has been freed.
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
**	 8-Jun-09 (gordy)
**	    Created.
*/

private void
checkValid()
    throws SQLException
{
    if ( buff == null )  throw SqlExFactory.get( ERR_GC4022_LOB_FREED );
    return;
} // checkValid


} // class BufferedClob

