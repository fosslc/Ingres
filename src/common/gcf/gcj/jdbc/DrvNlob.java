/*
** Copyright (c) 2009, 2010 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: DrvNlob.java
**
** Description:
**	Defines class which implements the JDBC Clob interface
**	for a DBMS NCS (UCS2) LOB Locator.
**
**  Classes:
**
**	DrvNlob
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	26-Feb-07 (gordy)
**	    Added enhanced performance access.
**	 4-May-07 (gordy)
**	    Set class access for reflection.
**      24-Nov-08 (rajus01) SIR 121238
**          - Added new JDBC 4.0 methods to avoid compilation errors with
**            JDK 1.6. The new methods return E_GC4019 error for now.
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to support
**            JDBC 4.0 SQLException hierarchy.
**      13-Apr-09 (rajus01) SIR 121238
**          This class now implements NClob to support NCS datatype.
**	16-Nov-10 (gordy)
**	    Added LOB locator streaming access.
*/

import	java.io.InputStream;
import	java.io.BufferedReader;
import	java.io.OutputStream;
import	java.io.Reader;
import	java.io.Writer;
import	java.io.IOException;
import	java.sql.NClob;
import	java.sql.Clob;
import	java.sql.SQLException;
import	com.ingres.gcf.util.DbmsConst;
import	com.ingres.gcf.util.SqlStream;
import	com.ingres.gcf.util.SqlExFactory;


/*
** Name: DrvNlob
**
** Description:
**	JDBC driver class which implements the JDBC Clob interface
**	for a DBMS NCS (UCS2) LOB Locator.
**
**  Interface Methods:
**
**	getAsciiStream		Retrieve stream which accesses LOB data.
**	getCharacterStream	Retrieve stream which accesses LOB data.
**	getSubString		Read characters from LOB data.
**	length			Get length of LOB data.
**	position		Search LOB data.
**
**    The following Clob interface methods are not supported by
**    locator based LOBs.
**
**	setAsciiStream		Write LOB data from stream.
**	setCharacterStream	Write LOB data from stream.
**	setString		Write characters into LOB data.
**	truncate		Set length of LOB data.
**
**  Public Methods:
**
**	get			Retrieve LOB data stream.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	26-Feb-07 (gordy)
**	    Added get() method providing enhanced performance 
**	    in retrieving LOB data under restricted conditions.
**	 4-May-07 (gordy)
**	    Class is not exposed outside package, restrict access.
*/

class	// package access
DrvNlob
    extends DrvLOB
    implements NClob, DbmsConst
{


/*
** Name: DrvNlob
**
** Description:
**	Class constructor.
**
** Input:
**	conn		DBMS Connection.
**	locator		DBMS LOB Locator.
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
DrvNlob( DrvConn conn, int locator )
{
    super( conn, DBMS_TYPE_LNLOC, locator );

    title = trace.getTraceName() + "-NClob[" + locator + "]";
    tr_id = "DrvNlob[" + locator + "]";
} // DrvNlob


/*
** Name:  free
**
** Description:
**      This method frees the Clob object and releases the resources the
**      resources that it holds. The object is invalid once the free method
**      is called.
**
**      After free has been called, any attempt to invoke a method other than
**      free will result in a SQLException being thrown. If free is called
**      multiple times, the subsequent calls to free are treated as a no-op.
**
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
**      24-Nov-08 (rajus01)
**          Created.
**	18-Jun-09 (rajus01)
**	    Implemented.
*/

public void
free() 
throws SQLException
{ 
    super.free();
    return;
} // free


/*
** Name: length
**
** Description:
**	Returns the length of the LOB data referenced
**	by the locator.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	long	Length of LOB.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public long
length()
    throws SQLException
{
    isValid();
    return( super.length() );
} // length


/*
** Name: getSubString
**
** Description:
**	Extracts a string from the LOB data referenced
**	by the locator.
**
** Input:
**	pos	Starting position.
**	length	Length of string.
**
** Output:
**	None.
**
** Returns:
**	String	Extracted string.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public String
getSubString( long pos, int length )
    throws SQLException
{
    isValid();
    return( super.getChars( pos, length, (length <= conn.max_nvch_len) ) );
} // getSubString


/*
** Name: position
**
** Description:
**	Search LOB data referenced by locator.
**
** Input:
**	pattern	Search pattern.
**	start	Starting position of search.
**
** Output:
**	None.
**
** Returns:
**	long	Position of match.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public long
position( String pattern, long start )
    throws SQLException
{
    if ( pattern == null )  throw new NullPointerException();
    isValid();
    long pos = super.position( pattern, start );
    return( (pos >= 1) ? pos : -1 );
} // position


/*
** Name: position
**
** Description:
**	Search LOB data referenced by locator.
**
** Input:
**	pattern	Search pattern.
**	start	Starting position of search.
**
** Output:
**	None.
**
** Returns:
**	long	Position of match.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public long
position( Clob pattern, long start )
    throws SQLException
{
    if ( pattern == null )  throw new NullPointerException();
    long pos = -1;
    isValid();

    if ( pattern instanceof DrvLOB  &&  
	 hasSameDomain( (DrvLOB)pattern ) )
	pos = position( (DrvLOB)pattern, start );
    else
	pos = super.position( pattern.getCharacterStream(), start );

    return( (pos >= 1) ? pos : -1 );
} // position


/*
** Name: getAsciiStream
**
** Description:
**	Returns a character InputStream which reads the 
**	LOB data referenced by the locator and converts 
**	the data into ASCII.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	InputStream	ASCII stream.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	26-Feb-07 (gordy)
**	    Set buffer size.  Request deferred stream.
**	16-Nov-10 (gordy)
**	    LOB locator access can either be done in separate
**	    requests for individual segments or as a single
**	    streaming data request.
*/

public InputStream
getAsciiStream()
    throws SQLException
{
    isValid();

    /*
    ** Generate a character stream representing the LOB
    ** and wrap it with a buffer to improve access.
    **
    ** Access can either be streaming or segmented.
    */
    boolean segment = ((conn.cnf_flags & conn.CNF_LOC_STRM) == 0);
    Reader rdr = new BufferedReader( super.getCharStream( segment ),
				     conn.cnf_lob_segSize );

    /*
    ** Wrap Reader with InputStream which
    ** performs the conversion into ASCII.
    */
    return( SqlStream.getAsciiIS( rdr ) );
} // getAsciiStream


/*
** Name: getCharacterStream
**
** Description:
**	Returns a character Reader which reads the 
**	LOB data referenced by the locator.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Reader	Character stream.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	26-Feb-07 (gordy)
**	    Set buffer size.  Request deferred stream.
**	16-Nov-10 (gordy)
**	    LOB locator access can either be done in separate
**	    requests for individual segments or as a single
**	    streaming data request.
*/

public Reader
getCharacterStream()
    throws SQLException
{
    isValid();

    /*
    ** Generate a character stream to represent the LOB
    ** and wrap it with a buffer to improve access.
    **
    ** Access can either be streaming or segmented.
    */
    boolean segment = ((conn.cnf_flags & conn.CNF_LOC_STRM) == 0);
    return( new BufferedReader( super.getCharStream( segment ),
				conn.cnf_lob_segSize ) );
} // getCharacterStream


/*
** Name: getCharacterStream
**
** Description:
**      Returns a character Reader that contains a partial Clob value,
**      starting with the character specified by pos, which is len characters
**      in length.
**
** Input:
**    pos   The offset to the first character of the partial value to be
**          retrieved. The first character in the Clob is at position 1.
**    len   The length in characters of the partial value to be retrieved.
**
** Output:
**      None.
**
** Returns:
**      Reader  Character stream.
**
** History:
**      24-Nov-08 (rajus01)
**          Created.
**	18-Jun-09 (rajus01)
**	    Implemented.
**	16-Nov-10 (gordy)
**	    LOB locator access can either be done in separate
**	    requests for individual segments or as a single
**	    streaming data request.
*/

public Reader
getCharacterStream(long pos, long len) 
throws SQLException
{ 
    isValid();

    /*
    ** Validate parameters.
    */
    if ( pos < 1  ||  len < 0 )
        throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );

    /*
    ** Generate a character stream to represent the LOB
    ** and wrap it with a buffer to improve access.
    **
    ** Access can either be streaming or segmented.
    */
    boolean segment = ((conn.cnf_flags & conn.CNF_LOC_STRM) == 0);
    return( new BufferedReader( super.getCharStream( segment, pos, len ),
				conn.cnf_lob_segSize ) );
} // getCharacterStream


/*
** Name: get
**
** Description:
**	Retrieves the LOB data associated with the
**	locator and returns a Reader which may be 
**	used to read the data.
**
**	The data stream must be read immediately
**	and completely consumed prior to any other
**	request being made on this Blob or the
**	associated connection.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Reader	Character stream.
**
** History:
**	26-Feb-07 (gordy)
**	    Created.
**	16-Nov-10 (gordy)
**	    Changed meaning of getByteStream() parameter.  Access is
**	    always deferred until InputStream is accessed.  Access
**	    can be segmented or streaming.  The latter is used here.
*/

public Reader
get()
    throws SQLException
{
    isValid();
    return( super.getCharStream( false ) );
} // get


/*
** The following methods are not supported due to the
** fact that DBMS LOB Locators are not updatable.
*/

public int
setString( long pos, String str )  throws SQLException
{ throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); }

public int
setString( long pos, String str, int offset, int len )  throws SQLException
{ throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); }

public OutputStream
setAsciiStream( long pos )  throws SQLException
{ throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); }

public Writer
setCharacterStream( long pos )  throws SQLException
{ throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); }

public void
truncate( long len )  throws SQLException
{ throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED ); }


} // class DrvNlob
